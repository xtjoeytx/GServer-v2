#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include <filesystem>

#include <CString.h>
#include <filesystem/FileSystem.h>
#include <scripting/ScriptContainers.h>

namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class LevelLoader
{
public:
	/// @brief Loads a level into the specified level pointer using the given level name path.
	///
	/// Always, always call Server::stubOrGetLevel() before calling this.
	/// @param level A pointer to the level object to be loaded.
	/// @param levelName The filesystem path representing the name or location of the level to load.
	/// @return A pointer to the loaded level object.
	static LevelPtr loadLevelInto(LevelPtr level, const std::filesystem::path& levelName);

private:
	static LevelPtr loadGraal(LevelPtr level, std::string_view fileVersion, fs::FileSystem* fileSystem, CString& fileData);
	static LevelPtr loadZelda(LevelPtr level, std::string_view fileVersion, fs::FileSystem* fileSystem, CString& fileData);
	static LevelPtr loadNW(LevelPtr level, std::string_view fileVersion, fs::FileSystem* fileSystem, CString& fileData);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELLOADER_H
