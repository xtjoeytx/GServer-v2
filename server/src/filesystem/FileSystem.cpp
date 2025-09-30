#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <generator>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <filesystem/watch/FileWatch.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs
{
///////////////////////////////////////////////////////////////////////////////

FileSystem::FileSystem(const std::filesystem::path& directory)
{
	bind(directory);
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
	m_watcher.add(directory, [this](uint32_t watch_id, const std::filesystem::path& dir, const std::filesystem::path& file, preagonal::fs::FileEventCollection e)
	{
		if (e.test(FileEvent::Invalid))
			return;

		FileData* eventFileData = nullptr;
		FileData deletedData;

		// Limit our lock to not include the event callbacks.
		{
			std::scoped_lock watchGuard{ m_file_mutex };
			auto iter = m_files.find(file.filename());

			// Existing file.
			if (iter != m_files.end())
			{
				std::unique_ptr<FileData> entry;

				// The file got changed.
				if (e.test(FileEvent::Modified))
				{
					// Check for no change in mod time.
					// Sometimes a modify event can get spawned multiple times.
					auto fileModTime = std::filesystem::last_write_time(dir / file);
					if (iter->second->modifiedTime == fileModTime)
						return;

					iter->second->modifiedTime = fileModTime;
				}

				// File got renamed, so copy the existing data to a new record.
				// There should be a file delete event coming up next, and adding this now might invalidate the iterators,
				// so we delay the actual insertion until after processing the delete event.
				if (e.test(FileEvent::Added))
					entry = std::make_unique<FileData>(*iter->second);

				// The file got deleted.
				if (e.test(FileEvent::Deleted))
				{
					// Make a copy of the data that is going to be deleted so we can pass it to the event callback.
					deletedData = *iter->second;
					eventFileData = &deletedData;

					m_files.erase(iter);
					iter = m_files.end();
				}

				// If we need to add a file, do it now.
				if (entry)
				{
					iter = m_files.insert(std::make_pair(file.filename(), std::move(entry)));
				}
			}
			// New file.
			else
			{
				if (e.test(FileEvent::Added))
				{
					auto entry = std::make_unique<FileData>();
					entry->file = dir / file;
					entry->file.make_preferred();
					entry->fileSize = std::filesystem::file_size(entry->file);
					entry->modifiedTime = std::filesystem::last_write_time(entry->file);
					assignCategoriesToFileData(*entry);

					iter = m_files.insert(std::make_pair(file.filename(), std::move(entry)));
				}
			}

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
	{
		std::scoped_lock guard{ m_file_mutex };
		for (auto& [filePath, info] : m_files)
		{
			if (string::equalsi(filePath.string(), fileName) && (skipTest || info->categories.test((size_t)category)))
				return true;
		}
	}

	return false;
}

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

std::generator<const FileData&> FileSystem::info(const std::filesystem::path& file) const
{
	std::scoped_lock guard{ m_file_mutex };

	auto iter = m_files.find(file);
	while (iter != m_files.end())
	{
		co_yield *iter->second.get();
		++iter;
	}
}

std::generator<const FileData&> FileSystem::info(FileCategory category) const
{
	std::scoped_lock guard{ m_file_mutex };

	bool skipTest = !hasFoldersConfig();
	for (auto& fileData : m_files)
	{
		if (skipTest || fileData.second->categories.test((size_t)category))
			co_yield *fileData.second.get();
	}
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

std::generator<const FileData&> FileSystem::infoi(const std::filesystem::path& file) const
{
	std::scoped_lock guard{ m_file_mutex };

	auto fileName = file.string();
	for (auto& [key, value] : m_files)
	{
		if (string::equalsi(key.string(), fileName))
			co_yield *value;
	}
}

std::shared_ptr<File> FileSystem::open(FileCategory category, const std::filesystem::path& file) const
{
	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
	{
		auto f = std::make_shared<File>(file);
		return f;
	}

	// Check if the file exists in the native file system and file is a filename we want to find.
	{
		std::scoped_lock guard{ m_file_mutex };

		if (auto fileData = info(category, file); fileData != nullptr)
			return std::make_shared<File>(fileData->file);
	}

	return nullptr;
}

std::generator<std::shared_ptr<File>> FileSystem::open(const std::filesystem::path& file) const
{
	// Check if the file exists in the native file system and file is a direct path.
	if (std::filesystem::exists(file))
	{
		auto f = std::make_shared<File>(file);
		co_yield f;
		co_return;
	}

	{
		std::scoped_lock guard{ m_file_mutex };

		for (auto& fileData : info(file))
			co_yield std::make_shared<File>(fileData.file);
	}
}

std::shared_ptr<File> FileSystem::open(const FileData& fileData) const
{
	if (std::filesystem::exists(fileData.file))
		return std::make_shared<File>(fileData.file);
	return nullptr;
}

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
			if (string::match(directory.native(), glob.native()))
				return static_cast<FileCategory>(i);
		}
	}
	return FileCategory::ALL;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs
