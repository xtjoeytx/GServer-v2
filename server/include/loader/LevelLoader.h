#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include <filesystem>

#include <common.h>
#include <level/Level.h>

namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class LevelLoader
{
public:
	static LevelPtr loadLevel(const std::filesystem::path& levelName);
	static LevelPtr loadLevelInto(LevelPtr level, const std::filesystem::path& levelName);

private:
	static LevelPtr loadGraal(LevelPtr level, FileSystem* fileSystem, CString& fileData);
	static LevelPtr loadZelda(LevelPtr level, FileSystem* fileSystem, CString& fileData);
	static LevelPtr loadNW(LevelPtr level, FileSystem* fileSystem, CString& fileData);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELLOADER_H
