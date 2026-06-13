#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <system_error>
#include <utility>
#include <vector>

#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <filesystem/watch/FileWatch.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/std/generator.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs
{
///////////////////////////////////////////////////////////////////////////////

FileSystem::FileSystem(const std::filesystem::path& directory)
{
	bind(directory);
}

FileSystem::~FileSystem()
{
	// Wait for any searching to finish.
	waitUntilFilesSearched();

	// Stop watching all directories.
	m_watcher.removeAll();

	// Mark ourselves as destructing so we can avoid callbacks.
	m_destructing = true;
}

//----------------------------

void FileSystem::reset()
{
	m_watcher.removeAll();

	// Clear our saved filesystem.
	std::scoped_lock guard{ m_file_mutex };
	m_files.clear();
	m_directories.clear();
}

void FileSystem::addFoldersConfigEntry(FileCategory category, const std::filesystem::path& glob)
{
	std::scoped_lock guard{ m_file_mutex };

	// Force into preferred format so we can do proper matching.
	std::filesystem::path preferred{ glob };
	preferred.make_preferred();

	// Add the glob to our folders config category.
	size_t categoryIndex = static_cast<size_t>(category);
	m_foldersConfig[categoryIndex].insert(preferred);

	// Apply the glob to any tracked files.
	for (auto& fileData : m_files)
	{
		if (string::match(fileData.second->file.native(), preferred.native()))
			fileData.second->categories.set(categoryIndex);
	}
}

void FileSystem::bind(const std::filesystem::path& directory)
{
	std::scoped_lock guard{ m_file_mutex };

	// If we are already watching this directory, abort.
	if (std::ranges::contains(m_directories, directory))
		return;

	// We are starting our file search.
	m_searching_files = true;

	// Create directories that don't exist.
	std::filesystem::create_directories(directory);

	// Fill our filesystem with file information.
	for (const auto& file : std::filesystem::recursive_directory_iterator(directory))
	{
		// If it is not a regular file, abort.
		if (!std::filesystem::is_regular_file(file.status()))
			continue;

		const auto& path = file.path();
		auto entry = std::make_unique<FileData>();

		entry->file = path;
		entry->file.make_preferred();

		entry->fileSize = file.file_size();
		entry->modifiedTime = file.last_write_time();
		assignCategoriesToFileData(*entry);

		m_files.insert(std::make_pair(path.filename(), std::move(entry)));
	}

	// We are done searching our file system.
	m_searching_files = false;
	m_searching_files_condition.notify_all();

	m_directories.insert(directory);
	m_watcher.add(directory, [this](uint32_t id, const std::filesystem::path& dir, const std::filesystem::path& file, const std::filesystem::path& oldFile, preagonal::fs::FileEventCollection e)
	{
		// Suppress the warning about unheld locks.  The static analysis is incorrect.
		#pragma warning(push)
		#pragma warning(disable: 26117)

		if (m_destructing || e.test(FileEvent::Invalid))
			return;

		FileData* eventFileData = nullptr;
		FileData deletedData;

		DEBUGPRINT("[FS] Event: {} on file: {} in dir: {}", e.to_string(), file.string(), dir.string());

		// Limit our lock to not include the event callbacks.
		{
			std::scoped_lock watchGuard{ m_file_mutex };
			auto iter = m_files.find(file.filename());

			// Existing file.
			if (iter != m_files.end())
			{
				// The file got changed.
				if (e.test(FileEvent::Modified) || e.test(FileEvent::Renamed))
				{
					if (std::filesystem::exists(dir / file))
					{
						// Check for no change in mod time.
						// Sometimes a modify event can get spawned multiple times.
						auto fileModTime = std::filesystem::last_write_time(dir / file);
						if (iter->second->modifiedTime == fileModTime)
							return;

						iter->second->modifiedTime = fileModTime;

						// If we got a rename event and overwrote an existing file,
						// we want to make sure we also have a modify event since the file got changed.
						if (!e.test(FileEvent::Modified))
							e.set(FileEvent::Modified);

						DEBUGPRINT("[FS] Existing file modified: {}", file.string());
					}
				}

				// The file got deleted.
				if (e.test(FileEvent::Deleted))
				{
					// Make a copy of the data that is going to be deleted so we can pass it to the event callback.
					deletedData = *iter->second;
					eventFileData = &deletedData;

					m_files.erase(iter);
					iter = m_files.end();

					DEBUGPRINT("[FS] Existing file deleted: {}", file.string());
				}

				// If the file got renamed, make sure we remove the old one from the file system.
				if (e.test(FileEvent::Renamed))
				{
					DEBUGPRINT("[FS] Existing file renamed: {} -> {}", oldFile.string(), file.string());
					auto oldIter = m_files.find(oldFile.filename());
					if (oldIter != m_files.end())
					{
						// Make a copy of the data that is going to be deleted so we can pass it to the event callback.
						deletedData = *oldIter->second;
						eventFileData = &deletedData;
						m_files.erase(oldIter);
						iter = m_files.find(file.filename());
						DEBUGPRINT("[FS] Old file deleted due to rename: {}", oldFile.string());
					}

					// If the old file was the same name as the new file, but with a .partial extension, then we ignore the renamed event.
					if (oldFile.extension() == ".partial" && oldFile.stem() == file)
					{
						e.reset(FileEvent::Renamed);
						DEBUGPRINT("[FS] Ignored renamed event for partial file: {} -> {}", oldFile.string(), file.string());
					}
				}
			}
			else
			{
				// File got renamed and didn't overwrite an existing file, so update the file entry internals.
				if (e.test(FileEvent::Renamed))
				{
					// Found the old entry.
					iter = m_files.find(oldFile.filename());
					if (iter != m_files.end())
					{
						// Update the file entry.
						iter->second->file = dir / file;
						iter->second->file.make_preferred();
						iter->second->modifiedTime = std::filesystem::last_write_time(iter->second->file);
						iter->second->categories.reset();
						assignCategoriesToFileData(*iter->second.get());

						// Change the key in the map.
						auto node = m_files.extract(iter);
						node.key() = file.filename();
						iter = m_files.insert(std::move(node));

						// Switch over to the added event since this is effectively a new file now.
						e.reset();
						e.set(FileEvent::Added);

						DEBUGPRINT("[FS] New file renamed from old: {} -> {}", oldFile.string(), file.string());
					}
					// Not found, so treat it as a new file.
					else
					{
						e.reset(FileEvent::Renamed);
						e.set(FileEvent::Added);

						DEBUGPRINT("[FS] New file added (old file not tracked): {} -> {}", oldFile.string(), file.string());
					}
				}

				// New file.
				if (iter == m_files.end() && e.test(FileEvent::Added) && std::filesystem::exists(dir / file))
				{
					auto entry = std::make_unique<FileData>();
					entry->file = dir / file;
					entry->file.make_preferred();
					entry->fileSize = std::filesystem::file_size(entry->file);
					entry->modifiedTime = std::filesystem::last_write_time(entry->file);
					assignCategoriesToFileData(*entry);

					iter = m_files.insert(std::make_pair(file.filename(), std::move(entry)));
					DEBUGPRINT("[FS] New file added: {}", file.string());
				}
			}

			// Extract the event data for callbacks.
			if (iter != m_files.end() && iter->second)
				eventFileData = iter->second.get();
		}

		// Run events.
		if (eventFileData != nullptr)
		{
			// File callbacks.
			if (eventFileData->eventCallback)
				eventFileData->eventCallback(e, *eventFileData);

			// Category callbacks.
			for (size_t i = 0; i < FileCategoryTypeCount; ++i)
			{
				if (eventFileData->categories.test(i) && categoryEventCallback[i])
					categoryEventCallback[i](e, *eventFileData);
			}
		}

		// Restore normal warnings
		#pragma warning(pop)
	});
}

void FileSystem::update()
{
	m_watcher.update();
}

//----------------------------

bool FileSystem::has(FileCategory category, const std::filesystem::path& file) const noexcept
{
	if (std::filesystem::exists(file))
		return true;

	// If we don't have a folders config, skip the category test.
	bool skipTest = !hasFoldersConfig();

	// Check if our file is saved in the file system list.
	{
		std::scoped_lock guard{ m_file_mutex };
		auto iter = m_files.find(file);
		if (iter != m_files.end())
		{
			if (iter->second != nullptr && (skipTest || iter->second->categories.test((size_t)category)))
				return true;
		}
	}

	return false;
}

bool FileSystem::has(const std::filesystem::path& file) const noexcept
{
	if (std::filesystem::exists(file))
		return true;

	// Check if our file is saved in the file system list.
	{
		std::scoped_lock guard{ m_file_mutex };
		if (auto iter = m_files.find(file); iter != m_files.end())
			return true;
	}

	return false;
}

bool FileSystem::hasi(FileCategory category, const std::filesystem::path& file) const noexcept
{
	if (std::filesystem::exists(file))
		return true;

	bool skipTest = !hasFoldersConfig();
	auto fileName = file.string();

	// Check if our file is saved in the file system list.
	std::scoped_lock guard{ m_file_mutex };
	for (auto& [filePath, info] : m_files)
	{
		if (string::equalsi(filePath.string(), fileName) && (skipTest || info->categories.test((size_t)category)))
			return true;
	}

	return false;
}

//----------------------------

std::filesystem::path FileSystem::find(FileCategory category, const std::filesystem::path& file) const noexcept
{
	if (auto fileInfo = info(category, file); fileInfo != nullptr)
		return fileInfo->file;

	return std::filesystem::path{};
}

std::filesystem::path FileSystem::findi(FileCategory category, const std::filesystem::path& file) const noexcept
{
	std::scoped_lock guard{ m_file_mutex };

	bool skipTest = !hasFoldersConfig();
	auto fileName = file.string();
	for (auto& [key, value] : m_files)
	{
		if ((skipTest || value->categories.test((size_t)category)) && string::equalsi(key.string(), fileName))
			return value->file;
	}
	return std::filesystem::path{};
}

//----------------------------

FileData* FileSystem::info(FileCategory category, const std::filesystem::path& file) const
{
	std::scoped_lock guard{ m_file_mutex };

	bool skipTest = !hasFoldersConfig();
	auto iter = m_files.find(file);
	while (iter != m_files.end())
	{
		if (skipTest || iter->second->categories.test((size_t)category))
			return iter->second.get();
		++iter;
	}
	return nullptr;
}

std::vector<FileDataWeakPtr> FileSystem::info(const std::filesystem::path& file) const
{
	std::scoped_lock guard{ m_file_mutex };
	std::vector<FileDataWeakPtr> result;

	for (auto& [key, value] : m_files)
	{
		if (key == file)
			result.push_back(value);
	}

	return result;
};

std::vector<FileDataWeakPtr> FileSystem::info(FileCategory category) const
{
	std::scoped_lock guard{ m_file_mutex };
	std::vector<FileDataWeakPtr> result;

	bool skipTest = !hasFoldersConfig();
	for (auto& fileData : m_files)
	{
		if (skipTest || fileData.second->categories.test((size_t)category))
			result.push_back(fileData.second);
	}

	return result;
}

FileData* FileSystem::infoi(FileCategory category, const std::filesystem::path& file) const
{
	std::scoped_lock guard{ m_file_mutex };

	bool skipTest = !hasFoldersConfig();
	auto fileName = file.string();
	for (auto& [key, value] : m_files)
	{
		if ((skipTest || value->categories.test((size_t)category)) && string::equalsi(key.string(), fileName))
			return value.get();
	}
	return nullptr;
}

std::vector<FileDataWeakPtr> FileSystem::infoi(const std::filesystem::path& file) const
{
	std::scoped_lock guard{ m_file_mutex };
	std::vector<FileDataWeakPtr> result;

	auto fileName = file.string();
	for (auto& [key, value] : m_files)
	{
		if (string::equalsi(key.string(), fileName))
			result.push_back(value);
	}

	return result;
}

//----------------------------

std::shared_ptr<File> FileSystem::open(FileCategory category, const std::filesystem::path& file) const
{
	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
	{
		if (auto f = std::make_shared<File>(file); f->opened())
			return f;
		return nullptr;
	}

	// Check if the file exists in the native file system and file is a filename we want to find.
	if (auto fileData = info(category, file); fileData != nullptr)
	{
		if (auto f = std::make_shared<File>(fileData->file); f->opened())
			return f;
	}

	return nullptr;
}

std::vector<std::shared_ptr<File>> FileSystem::open(const std::filesystem::path& file) const
{
	std::vector<std::shared_ptr<File>> result;

	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
	{
		auto f = std::make_shared<File>();
		f->setFilePath(file);
		result.push_back(f);
		return result;
	}

	for (auto& [fileName, fileData] : m_files)
	{
		auto f = std::make_shared<File>();
		f->setFilePath(fileName);
		result.push_back(f);
	}

	return result;
}

std::shared_ptr<File> FileSystem::open(const FileData& fileData) const
{
	if (std::filesystem::exists(fileData.file))
	{
		if (auto f = std::make_shared<File>(fileData.file); f->opened())
			return f;
	}

	return nullptr;
}

std::shared_ptr<File> FileSystem::openi(FileCategory category, const std::filesystem::path& file) const
{
	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
	{
		if (auto f = std::make_shared<File>(file); f->opened())
			return f;
		return nullptr;
	}

	// Check if the file exists in the native file system and file is a filename we want to find.
	if (auto fileData = infoi(category, file); fileData != nullptr)
	{
		if (auto f = std::make_shared<File>(fileData->file); f->opened())
			return f;
	}

	return nullptr;
}

//----------------------------

std::shared_ptr<FileIO> FileSystem::openForWriting(FileCategory category, const std::filesystem::path& file, bool createNew) const
{
	FileIOPtr result = nullptr;

	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
		result = std::make_shared<FileIO>(file);

	// Check if the file exists in the native file system and file is a filename we want to find.
	if (result == nullptr)
	{
		if (auto fileData = info(category, file); fileData != nullptr)
			result = std::make_shared<FileIO>(fileData->file);
	}

	// If we have a file, return it.
	if (result != nullptr)
	{
		if (result->opened())
			return result;
		return nullptr;
	}
	if (!createNew)
		return nullptr;

	// Create the new file.
	auto directories = getManagedDirectories(category);
	auto first = directories.begin();
	if (first == directories.end())
		return nullptr;

	return std::make_shared<FileIO>((*first) / file);
}

std::vector<std::shared_ptr<FileIO>> FileSystem::openForWriting(const std::filesystem::path& file) const
{
	std::vector<std::shared_ptr<FileIO>> result;

	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
	{
		auto f = std::make_shared<FileIO>();
		f->setFilePath(file);
		result.push_back(f);
		return result;
	}

	for (auto& [fileName, fileData] : m_files)
	{
		auto f = std::make_shared<FileIO>();
		f->setFilePath(fileName);
		result.push_back(f);
	}

	return result;
}

std::shared_ptr<FileIO> FileSystem::openForWriting(const FileData& fileData) const
{
	if (std::filesystem::exists(fileData.file))
	{
		if (auto f = std::make_shared<FileIO>(fileData.file); f->opened())
			return f;
	}

	return nullptr;
}

std::shared_ptr<FileIO> FileSystem::openiForWriting(FileCategory category, const std::filesystem::path& file, bool createNew) const
{
	FileIOPtr result = nullptr;

	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
		result = std::make_shared<FileIO>(file);

	// Check if the file exists in the native file system and file is a filename we want to find.
	if (result == nullptr)
	{
		if (auto fileData = infoi(category, file); fileData != nullptr)
			result = std::make_shared<FileIO>(fileData->file);
	}

	// If we have a file, return it.
	if (result != nullptr)
	{
		if (result->opened())
			return result;
		return nullptr;
	}
	if (!createNew)
		return nullptr;

	// Create the new file.
	auto directories = getManagedDirectories(category);
	auto first = directories.begin();
	if (first == directories.end())
		return nullptr;

	return std::make_shared<FileIO>((*first) / file);
}

//----------------------------

void FileSystem::addExisting(FileCategory category, const std::filesystem::path& fullFilePath)
{
	if (!std::filesystem::exists(fullFilePath))
		return;

	std::scoped_lock guard{m_file_mutex};

	auto files = m_files.find(fullFilePath.filename());
	if (files != m_files.end())
		return;

	auto entry = std::make_unique<FileData>();
	entry->file = fullFilePath;
	entry->file.make_preferred();
	entry->fileSize = std::filesystem::file_size(entry->file);
	entry->modifiedTime = std::filesystem::last_write_time(entry->file);
	assignCategoriesToFileData(*entry);

	m_files.insert(std::make_pair(fullFilePath.filename(), std::move(entry)));
}

//----------------------------

FileData* FileSystem::rename(const FileData& fileData, std::filesystem::path newFileName)
{
	if (!std::filesystem::exists(fileData.file))
		return nullptr;

	std::scoped_lock guard{ m_file_mutex };

	auto files = m_files.find(fileData.file.filename());
	while (files != m_files.end())
	{
		if (files->second->modifiedTime == fileData.modifiedTime && files->second->categories == fileData.categories)
		{
			auto newFilePath = fileData.file.parent_path() / newFileName;
			newFilePath.make_preferred();

			// Rename the file.
			std::error_code ec;
			std::filesystem::rename(fileData.file, newFilePath, ec);
			if (ec)
			{
				log::printLine(log::server, "** Error renaming file [{}] to [{}]: {} **", fileData.file.filename().string(), newFileName.string(), ec.message());
				return nullptr;
			}

			// Update the file data.
			files->second->file = newFilePath;
			files->second->refreshModTime();

			// Reset the categories.
			files->second->categories.reset();
			assignCategoriesToFileData(*files->second.get());

			// Update the key.
			auto node = m_files.extract(files);
			node.key() = newFilePath.filename();
			files = m_files.insert(std::move(node));

			// Return our new data.
			return files->second.get();
		}
	}

	return nullptr;
}

//----------------------------

std::generator<const std::filesystem::path&> FileSystem::getManagedDirectories() const
{
	for (const auto& dir : m_directories)
		co_yield dir;
}

std::generator<const std::filesystem::path&> FileSystem::getManagedDirectories(FileCategory category) const
{
	bool skipTest = !hasFoldersConfig();
	if (skipTest || category == FileCategory::ALL)
		co_yield std::ranges::elements_of(getManagedDirectories());

	for (const auto& dir : m_directories)
	{
		if (categoryForDirectory(dir) == category)
			co_yield dir;
	}
}

//----------------------------

void FileSystem::assignCategoriesToFileData(FileData& fileData)
{
	fileData.categories.set(ENUM(FileCategory::ALL));

	for (size_t i = 0; i < FileCategoryTypeCount; ++i)
	{
		if (m_foldersConfig[i].empty())
			continue;

		for (const auto& glob : m_foldersConfig[i])
		{
			if (string::match(fileData.file.native(), glob.native()))
			{
				fileData.categories.set(i);
				break;
			}
		}
	}
}

FileCategory FileSystem::categoryForDirectory(const std::filesystem::path& directory) const
{
	for (size_t i = 0; i < FileCategoryTypeCount; ++i)
	{
		if (m_foldersConfig[i].empty())
			continue;

		for (const auto& glob : m_foldersConfig[i])
		{
			if (string::match(directory.native(), glob.parent_path().native()))
				return static_cast<FileCategory>(i);
		}
	}
	return FileCategory::ALL;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs
