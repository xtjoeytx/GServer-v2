#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include <concepts>
#include <filesystem/File.h>
#include <filesystem/FileSystemTypes.h>
#include <filesystem/watch/FileWatch.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs
{
///////////////////////////////////////////////////////////////////////////////

struct FileData;

/// @brief A callback function for file system events.
using FileEventCallback = std::function<void(FileEventCollection, FileData&)>;

//----------------------------

/// @brief Data describing a file.
struct FileData
{
	/// @brief The full path to the file.
	std::filesystem::path file;

	/// @brief The size of the file.
	uintmax_t fileSize = 0;

	/// @brief The categories this file belongs to.
	FileCategoryCollection categories;

	/// @brief The file's modified time.
	std::filesystem::file_time_type modifiedTime;

	/// @brief A callback function for handling file system events for a specific file.
	FileEventCallback eventCallback;

	/// @brief Retrieves the last modification time of the file.
	/// @return A time_point representing the last write time of the file.
	clock::time_point getModTime() const
	{
		return getFileModTime(file);
	}

	/// @brief Sets the last modification time of a file.
	/// @param modTime The new modification time to set.
	void setModTime(clock::time_point modTime) const
	{
		std::filesystem::last_write_time(file, toFileClock(modTime));
	}

	/// @brief Updates the modified time with the last modification time of the file.
	void refreshModTime()
	{
		modifiedTime = std::filesystem::last_write_time(file);
	}

	/// @brief Deletes the file associated with the object.
	/// @return true if the file was successfully deleted; false otherwise.
	bool deleteFile() const
	{
		return std::filesystem::remove(file);
	}

	/// @brief Opens the file associated with the object.
	/// @return A shared pointer to the opened file.
	std::shared_ptr<File> openFile() const
	{
		return std::make_shared<File>(file);
	}

	/// @brief Opens the file associated with the object for writing.
	/// @return A shared pointer to the opened file.
	std::shared_ptr<FileIO> openFileForWriting() const
	{
		return std::make_shared<FileIO>(file);
	}
};
using FileDataPtr = std::shared_ptr<FileData>;
using FileDataWeakPtr = std::weak_ptr<FileData>;

//----------------------------

/// @brief Manages files within a directory.
class FileSystem
{
public:
	FileSystem() = default;
	FileSystem(const std::filesystem::path& directory);
	~FileSystem();

	FileSystem(const FileSystem& other) = delete;
	FileSystem(FileSystem&& other) = delete;
	FileSystem& operator=(const FileSystem& other) = delete;
	FileSystem& operator=(FileSystem&& other) = delete;

public:
	/// @brief Resets the file system.
	void reset();

	/// @brief Sets the folders configuration.
	void addFoldersConfigEntry(FileCategory category, const std::filesystem::path& glob);

	/// @brief Binds to a directory.
	/// @param directory The directory to bind to.
	void bind(const std::filesystem::path& directory);

	/// @brief Binds to multiple directories.
	void bind(string::StringVariant auto... directories)
	{
		(..., bind(std::filesystem::path{ directories }));
	}

	/// @brief Binds to multiple directories.
	/// @param directories A range of directories of type std::filesystem::path.
	void bind(std::ranges::input_range auto&& directories)
		requires std::same_as<std::ranges::range_value_t<decltype(directories)>, std::filesystem::path>
	{
		for (const auto& path : directories)
			bind(path);
	}

	/// @brief Binds to multiple directories.
	/// @param directories A range of directories of type string::StringViewIshVariant.
	void bind(std::ranges::input_range auto&& directories)
		requires string::StringViewIshVariant<std::ranges::range_value_t<decltype(directories)>>
	{
		for (const auto& path : directories)
			bind(std::filesystem::path{ path });
	}

	/// @brief Binds to a directory in a non-recursive manner.
	/// @param directory The directory to bind to.
	void bindSingleFile(const std::filesystem::path& file);

	/// @brief Checks for changes to the underlying OS filesystem.  Call this every so often.
	void update();

public:
	/// @brief Checks if the filesystem is empty.
	/// @return True if the filesystem is empty, false if not.
	[[inline]] bool empty() const noexcept;

	/// @brief Checks if the file exists in the filesystem.
	/// @param category The category the file must belong to.
	/// @param file The file name to check for.
	/// @return True if the file exists, false if not.
	bool has(FileCategory category, const std::filesystem::path& file) const noexcept;

	/// @brief Checks if the file exists in the filesystem.
	/// @param file The file name to check for.
	/// @return True if the file exists, false if not.
	bool has(const std::filesystem::path& file) const noexcept;

	/// @brief Checks if the file exists in the filesystem (case-insensitive).
	/// @param category The category the file must belong to.
	/// @param file The file name to check for.
	/// @return True if the file exists, false if not.
	bool hasi(FileCategory category, const std::filesystem::path& file) const noexcept;

	/// @brief Checks whether a folders configuration is present.
	/// @return True if a folders configuration exists; otherwise, false.
	[[inline]] bool hasFoldersConfig() const noexcept;

public:
	/// @brief Finds a file path corresponding to the specified file category and file.
	/// @param category The category of the file to find.
	/// @param file The file path to search for, provided as a reference to a std::filesystem::path object.
	/// @return A std::filesystem::path representing the found file path corresponding to the given category and file.
	std::filesystem::path find(FileCategory category, const std::filesystem::path& file) const noexcept;

	/// @brief Finds a file path corresponding to the specified file category and file (case-insensitive).
	/// @param category The category of the file to find.
	/// @param file The file path to search for, provided as a reference to a std::filesystem::path object.
	/// @return A std::filesystem::path representing the found file path corresponding to the given category and file.
	std::filesystem::path findi(FileCategory category, const std::filesystem::path& file) const noexcept;

public:
	/// @brief Returns information about the file.
	/// @param category The category the file must belong to.
	/// @return Information about the file.
	FileData* info(FileCategory category, const std::filesystem::path& file) const;

	/// @brief Returns information about the file.
	/// @return Information about the file.
	std::vector<FileDataWeakPtr> info(const std::filesystem::path& file) const;

	/// @brief Gets a range of all files in a category.
	std::vector<FileDataWeakPtr> info(FileCategory category) const;

	/// @brief Returns information about the file (case-insensitive).
	/// @param category The category the file must belong to.
	/// @return Information about the file.
	FileData* infoi(FileCategory category, const std::filesystem::path& file) const;

	/// @brief Returns information about the file (case-insensitive).
	/// @param category The category the file must belong to.
	/// @return Information about the file.
	std::vector<FileDataWeakPtr> infoi(const std::filesystem::path& file) const;

public:
	/// @brief Opens a file by name.
	/// @param category The category the file must belong to.
	/// @param file The file name to open.
	/// @return A shared pointer to the file.
	std::shared_ptr<File> open(FileCategory category, const std::filesystem::path& file) const;

	/// @brief Opens multiple files by name.
	/// @param file The file name to open.
	/// @return A shared pointer to the file.
	std::vector<std::shared_ptr<File>> open(const std::filesystem::path& file) const;

	/// @brief Opens a file from the file data.
	/// @param fileData The file data of the file to open.
	/// @return A shared pointer to the file.
	std::shared_ptr<File> open(const FileData& fileData) const;

	/// @brief Opens a file by name (case-insensitive).
	/// @param category The category the file must belong to.
	/// @param file The file name to open.
	/// @return A shared pointer to the file.
	std::shared_ptr<File> openi(FileCategory category, const std::filesystem::path& file) const;

public:
	/// @brief Opens a file by name for writing.
	/// @param category The category the file must belong to.
	/// @param file The file name to open.
	/// @param createNew If true, and the file does not exist, it creates a new file in the first directory of the specified category.
	/// @return A shared pointer to the file.
	std::shared_ptr<FileIO> openForWriting(FileCategory category, const std::filesystem::path& file, bool createNew = false) const;

	/// @brief Opens multiple files by name for writing.
	/// @param file The file name to open.
	/// @return A shared pointer to the file.
	std::vector<std::shared_ptr<FileIO>> openForWriting(const std::filesystem::path& file) const;

	/// @brief Opens a file from the file data for writing.
	/// @param fileData The file data of the file to open.
	/// @return A shared pointer to the file.
	std::shared_ptr<FileIO> openForWriting(const FileData& fileData) const;

	/// @brief Opens a file by name for writing (case-insensitive).
	/// @param category The category the file must belong to.
	/// @param file The file name to open.
	/// @param createNew If true, and the file does not exist, it creates a new file in the first directory of the specified category.
	/// @return A shared pointer to the file.
	std::shared_ptr<FileIO> openiForWriting(FileCategory category, const std::filesystem::path& file, bool createNew = false) const;

public:
	/// @brief Creates an entry for a file in the specified category.  This is used to create an entry for a file we are creating, but don't want the file watcher to process an add event.
	/// @param category The category the file must belong to.
	/// @param file The file name to create a stub for.
	void addExisting(FileCategory category, const std::filesystem::path& fullFilePath);

public:
	/// @brief Renames a file to a new file path.
	/// @param fileData The data structure containing information about the file to be renamed.
	/// @param newFilePath The new path for the file.
	/// @return A pointer to the updated FileData structure if the rename was successful; otherwise, nullptr.
	FileData* rename(const FileData& fileData, std::filesystem::path newFilePath);

public:
	/// @brief Returns a generator that yields references to the managed directories.
	/// @return A generator that produces references to each managed directory.
	std::generator<const std::filesystem::path&> getManagedDirectories() const;
	std::generator<const std::filesystem::path&> getManagedDirectories(FileCategory category) const;

public:
	/// @brief Returns true if we are searching the filesystem.
	[[inline]] bool isSearchingForFiles() const;

	/// @brief Blocks the thread until files have been fully searched.
	[[inline]] void waitUntilFilesSearched();

public:
	/// @brief An array that stores a callback function for each file category type.
	std::array<FileEventCallback, FileCategoryTypeCount> categoryEventCallback;

private:
	void defaultWatchCallback(uint32_t id, const std::filesystem::path& dir, const std::filesystem::path& file, const std::filesystem::path& oldFile, fs::FileEventCollection e);
	void assignCategoriesToFileData(FileData& fileData);
	FileCategory categoryForDirectory(const std::filesystem::path& directory) const;

private:
	watch::FileWatch m_watcher;
	std::atomic<bool> m_searching_files;
	std::condition_variable m_searching_files_condition;
	std::unordered_set<std::filesystem::path> m_directories;
	std::unordered_set<std::filesystem::path> m_foldersConfig[FileCategoryTypeCount];
	std::unordered_multimap<std::filesystem::path, FileDataPtr> m_files;

	bool m_destructing = false;
	mutable std::mutex m_file_mutex;
};

//----------------------------

inline bool FileSystem::empty() const noexcept
{
	return m_files.empty();
}

inline bool FileSystem::hasFoldersConfig() const noexcept
{
	return std::ranges::any_of(m_foldersConfig, [](const auto& cfg)
	{
		return !cfg.empty();
	});
}

inline bool FileSystem::isSearchingForFiles() const
{
	return m_searching_files;
}

inline void FileSystem::waitUntilFilesSearched()
{
	if (!m_searching_files)
		return;

	std::unique_lock guard(m_file_mutex);
	m_searching_files_condition.wait(guard);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs

#endif // FILESYSTEM_H
