#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <level/Level.h>
#include <scripting/ScriptContainers.h>
#include <utilities/Extents.h>

namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class LevelLoader
{
public:
	/// @brief Loads a level from the specified file path.
	/// @param levelName The filesystem path representing the name or location of the level to load.
	/// @return A shared pointer to the loaded level object.
	static LevelPtr loadLevel(const std::filesystem::path& levelName);

	/// @brief Loads a level from the specified file path into an existing level object.
	/// @param levelName The filesystem path representing the name or location of the level to load.
	/// @param level The shared pointer to the level object where the loaded data will be stored.
	/// @return True if the level was loaded successfully, false otherwise.
	static bool loadLevelInto(const std::filesystem::path& levelName, const LevelPtr& level);

public:
	/// @brief Loads the static data of a specified level using the given level name.
	/// @param levelName The filesystem path representing the name or location of the level to load.
	/// @return A shared pointer to the loaded static level data object.
	static StaticLevelDataPtr loadStaticData(const std::filesystem::path& levelName);

	/// @brief Loads the static data of a specified level into an existing static level data object.
	/// @param staticLevelData The pointer to the static level data object where the loaded data will be stored.
	/// @return True if the static level data was loaded successfully, false otherwise.
	static bool loadStaticDataInto(const StaticLevelDataPtr& staticLevelData);

	/// @brief Attaches static level data to a given level, returning a SubLevel that attaches it.
	/// @param level The level to attach the static data to.
	/// @param mapPosition The location within the map where the static data should be applied.
	/// @param staticData The static data to attach to the level.
	/// @return A new SubLevelPtr that represents the attached static data within the level.
	static SubLevelPtr attachStaticDataToLevel(const LevelPtr& level, std::optional<MapPosition> mapPosition, const StaticLevelDataPtr& staticData);

	/// @brief Loads the NPCs for a given level from the provided static level data.
	/// @param level The shared pointer to the level for which NPCs are to be loaded.
	/// @param mapPosition An optional map position indicating the specific sub-level location.
	/// @param staticData A pointer to the static level data containing NPC information.
	static void loadStaticDataNPCs(const LevelPtr& level, std::optional<MapPosition> mapPosition, const StaticLevelDataPtr& staticData);

private:
	static void loadBinaryTiles(const StaticLevelDataPtr& levelData, const fs::FilePtr& fileData, uint32_t bits, uint8_t layers);
	static void loadBinaryLinks(const StaticLevelDataPtr& levelData, const fs::FilePtr& fileData, const fs::FileSystem& fileSystem);
	static void loadBinaryBaddies(const StaticLevelDataPtr& levelData, const fs::FilePtr& fileData, bool loadVerses);
	static void loadBinaryNPCs(const StaticLevelDataPtr& levelData, const fs::FilePtr& fileData);
	static void loadBinaryChests(const StaticLevelDataPtr& levelData, const fs::FilePtr& fileData);
	static void loadBinarySigns(const StaticLevelDataPtr& levelData, const fs::FilePtr& fileData);

private:
	static bool loadGraal(const StaticLevelDataPtr& levelData, std::string_view fileVersion, fs::FileSystem& fileSystem, fs::FilePtr& fileData);
	static bool loadZelda(const StaticLevelDataPtr& levelData, std::string_view fileVersion, fs::FileSystem& fileSystem, fs::FilePtr& fileData);
	static bool loadNW(const StaticLevelDataPtr& levelData, std::string_view fileVersion, fs::FileSystem& fileSystem, fs::FilePtr& fileData);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELLOADER_H
