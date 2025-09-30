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
	static LevelPtr loadLevel(const std::filesystem::path& levelName);
	static LevelPtr loadLevelInto(LevelPtr level, const std::filesystem::path& levelName);

private:
	static LevelPtr loadGraal(LevelPtr level, std::string_view fileVersion, fs::FileSystem* fileSystem, CString& fileData);
	static LevelPtr loadZelda(LevelPtr level, std::string_view fileVersion, fs::FileSystem* fileSystem, CString& fileData);
	static LevelPtr loadNW(LevelPtr level, std::string_view fileVersion, fs::FileSystem* fileSystem, CString& fileData);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELLOADER_H
