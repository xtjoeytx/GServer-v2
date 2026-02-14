#ifndef FILESYSTEMTYPES_H
#define FILESYSTEMTYPES_H

#include <bitset>
#include <chrono>
#include <cstdint>
#include <filesystem>

#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs
{
///////////////////////////////////////////////////////////////////////////////

/// @brief Retrieves the last modification time of a file as a clock::time_point.
/// @param file The path to the file whose modification time is to be retrieved.
/// @return The time point representing the last modification time of the specified file, using the clock type 'clock'.
inline clock::time_point getFileModTime(const std::filesystem::path& file)
{
	return toSystemClock(std::filesystem::last_write_time(file));
}

//----------------------------

/// @brief A category assigned to a file.
enum class FileCategory : uint8_t
{
	ALL = 0,
	//
	FILE,
	LEVEL,
	HEAD,
	BODY,
	SWORD,
	SHIELD,
	SOUND,
	//
	ACCOUNT,
	CONFIG,
	NPC,
	SCRIPTCLASS,
	TRANSLATION,
	WEAPON,
	//
	COUNT
};
inline constexpr uint8_t FileCategoryTypeCount = static_cast<uint8_t>(FileCategory::COUNT);
using FileCategoryCollection = std::bitset<FileCategoryTypeCount>;

//----------------------------

/// @brief An event that occurs in the file system.
struct FileEvent
{
	static constexpr uint8_t Invalid = 0;
	static constexpr uint8_t Added = 1;
	static constexpr uint8_t Deleted = 2;
	static constexpr uint8_t Modified = 3;
	static constexpr uint8_t Renamed = 4;
};

inline constexpr size_t FileEventTypeCount = 5;
using FileEventCollection = std::bitset<FileEventTypeCount>;

//----------------------------

/// @brief Represents data related to file events.
struct FileEventData
{
	FileEventCollection events;
	std::filesystem::path fileName;
	std::filesystem::path oldFileName;
	void* fsData = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs

#endif // FILESYSTEMTYPES_H
