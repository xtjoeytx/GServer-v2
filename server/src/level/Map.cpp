#include <cmath>
#include <cstdint>
#include <filesystem>
#include <generator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <CString.h>

#include <BabyDI.h>
#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <level/Map.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

Map::Map(is_bigmap_t, const std::filesystem::path& fileName)
	: mapType(MapType::BIGMAP), fileName(fileName)
{
	// Get the appropriate filesystem.
	auto server = BabyDI::Get<Server>();
	FileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings().getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_FILE);

	CString fullFilePath = fileSystem->find(fileName.string());

	// Make sure the file exists.
	if (fullFilePath.length() == 0)
		throw std::runtime_error("Map file not found!");

	// Stupid.
	auto& constructSize = const_cast<Dimension<uint8_t>&>(size);
	auto& constructLevels = const_cast<string_map<Position<uint8_t>>&>(levels);
	Position<uint8_t> currentPosition;

	// Load the levels.
	auto fileData = CString::loadToken(fullFilePath);
	for (auto& line : fileData)
	{
		line = line.removeAll("\r").trim();
		if (line.isEmpty())
			continue;

		auto levelList = string::fromCSV(line.toStringView());
		int empty = 0;
		for (const auto& lvl : levelList)
		{
			if (!lvl.empty())
			{
				constructLevels.insert({ string::toLower(lvl), currentPosition });
				levelsByName.insert({ string::toLower(lvl), std::weak_ptr<Level>() });
			}
			else ++empty;

			++currentPosition.x();
		}
		currentPosition.x() = 0;
		++currentPosition.y();

		// Calculate width/height.
		auto currentWidth = levelList.size() - empty;
		++constructSize.height();
		if (constructSize.width() < currentWidth)
			constructSize.width() = currentWidth;
	}

	// Size the positional storage.
	levelsByPosition.resize(static_cast<size_t>(constructSize.width() * constructSize.height()));
}

Map::Map(is_gmap_t, const std::filesystem::path& fileName)
	: mapType(MapType::GMAP), fileName(fileName)
{
	// Get the appropriate filesystem.
	auto server = BabyDI::Get<Server>();
	FileSystem* fileSystem = server->getFileSystem();
	if (!server->getSettings().getBool("nofoldersconfig", false))
		fileSystem = server->getFileSystem(FS_LEVEL);

	CString fullFilePath = fileSystem->find(fileName.string());

	// Make sure the file exists.
	if (fullFilePath.length() == 0)
		return;

	// Stupid.
	auto& constructSize = const_cast<Dimension<uint8_t>&>(size);
	auto& constructLevels = const_cast<string_map<Position<uint8_t>>&>(levels);
	auto& constructPreload = const_cast<string_set&>(levelsToKeepInMemory);
	Position<uint8_t> currentPosition;

	// Load the gmap.
	auto fileData = CString::loadToken(fullFilePath);
	for (auto it = fileData.begin(); it != fileData.end(); ++it)
	{
		// Tokenize
		auto curLine = string::splitHard(string::trim(it->toStringView()));
		if (curLine.empty())
			continue;

		// Parse Each Type
		if (curLine[0] == "WIDTH")
		{
			if (curLine.size() != 2)
				continue;

			constructSize.width() = string::toNumber(curLine[1]);
		}
		else if (curLine[0] == "HEIGHT")
		{
			if (curLine.size() != 2)
				continue;

			constructSize.height() = string::toNumber(curLine[1]);
		}
		else if (curLine[0] == "GENERATED")
		{
			if (curLine.size() != 2)
				continue;

			// Not really needed.
		}
		else if (curLine[0] == "LEVELNAMES")
		{
			++it;
			currentPosition.y() = 0;

			while (it != fileData.end())
			{
				auto lines = string::fromCSV(string::trim(it->toStringView()));
				if (lines.empty())
				{
					++it;
					continue;
				}
				if (lines[0] == "LEVELNAMESEND") break;

				if (currentPosition.y() < constructSize.height())
				{
					for (auto& levelName : lines)
					{
						if (currentPosition.x() < constructSize.width())
						{
							if (!levelName.empty())
								constructLevels.insert({ string::toLower(levelName), currentPosition });

							++currentPosition.x();
						}
					}

					currentPosition.x() = 0;
					++currentPosition.y();
				}

				++it;
			}
		}
		else if (curLine[0] == "MAPIMG")
		{
			if (curLine.size() != 2)
				continue;

			const_cast<std::string&>(mapImage) = curLine[1];
		}
		else if (curLine[0] == "MINIMAPIMG")
		{
			if (curLine.size() != 2)
				continue;

			const_cast<std::string&>(miniMapImage) = curLine[1];
		}
		else if (curLine[0] == "NOAUTOMAPPING")
		{
			// Clientside only.
		}
		else if (curLine[0] == "LOADFULLMAP")
		{
			const_cast<bool&>(keepAllLevelsLoaded) = true;
		}
		else if (curLine[0] == "LOADATSTART")
		{
			const_cast<bool&>(keepAllLevelsLoaded) = false;

			++it;
			while (it != fileData.end())
			{
				auto lines = string::fromCSV(string::trim(it->toStringView()));
				if (lines.empty())
				{
					++it;
					continue;
				}
				if (lines[0] == "LOADATSTARTEND") break;

				for (auto& levelName : lines)
					constructPreload.emplace(string::toLower(levelName));
			}
		}
		// TODO: 3D settings maybe?
	}

	// Size the positional storage.
	levelsByPosition.resize(static_cast<size_t>(constructSize.width()* constructSize.height()));
}

//----------------------------

void Map::loadMapLevels() const
{
	auto server = BabyDI::Get<Server>();
	if (keepAllLevelsLoaded)
	{
		auto server = BabyDI::Get<Server>();
		for (const auto& [levelName, position] : levels)
		{
			if (auto level = server->getLevel(levelName); level != nullptr)
			{
				auto index = position.x() + position.y() * size.width();
				levelsByName[levelName] = level;
				levelsByPosition[index] = level;
			}
		}
	}
	else if (!levelsToKeepInMemory.empty())
	{
		for (const auto& levelName : levelsToKeepInMemory)
		{
			if (auto level = server->getLevel(levelName); level != nullptr)
			{
				auto index = level->mapPosition.x() + level->mapPosition.y() * size.width();
				levelsByName[levelName] = level;
				levelsByPosition[index] = level;
			}
		}
	}
}

//----------------------------

bool Map::hasLevel(std::string_view levelName) const
{
	auto it = levels.find(levelName);
	return it != levels.end();
}

std::optional<Position<uint8_t>> Map::getLevelPosition(std::string_view levelName) const
{
	auto it = levels.find(levelName);
	if (it != levels.end())
		return it->second;
	return std::nullopt;
}

std::shared_ptr<Level> Map::getLevelAt(int x, int y) const
{
	for (const auto& [levelName, levelPos] : levels)
	{
		if (levelPos.x() == x && levelPos.y() == y)
			return getLevelPtr(levelName, levelsByName[levelName]);
	}
	return nullptr;
}

std::shared_ptr<Level> Map::getLevelAt(const PixelPosition& globalPosition) const
{
	int x = static_cast<int>(std::floor(globalPosition.x() / 1024));
	int y = static_cast<int>(std::floor(globalPosition.y() / 1024));
	return getLevelAt(x, y);
}

std::generator<std::shared_ptr<Level>> Map::getLevelsInRange(const TilePosition& position, int syncx, int syncy) const noexcept
{
	Position<int16_t> searchPos{ static_cast<int16_t>(position.x() / 64), static_cast<int16_t>(position.y() / 64) };
	Dimension<uint8_t> levelSyncDistance{ static_cast<uint8_t>(std::ceilf(syncx / 64)), static_cast<uint8_t>(std::ceilf(syncy / 64)) };
	Rectangle<int16_t, uint8_t> area{ searchPos.translate(-levelSyncDistance.width(), -levelSyncDistance.height()), levelSyncDistance * 2 };

	for (const auto& [levelName, levelPos] : levels)
	{
		if (levelPos.x() >= area.left() && levelPos.x() < area.right() &&
			levelPos.y() >= area.top() && levelPos.y() < area.bottom())
		{
			if (auto level = getLevelPtr(levelName, levelsByName[levelName]); level != nullptr)
				co_yield level;
		}
	}
}

std::generator<std::shared_ptr<Level>> Map::getLevelsInRectangle(const PixelRectangleArea& area) const noexcept
{
	for (const auto& [levelName, levelPtr] : levelsByName)
	{
		if (auto level = getLevelPtr(levelName, levelPtr); level != nullptr)
		{
			auto levelBox = level->getMapBoundingBox();
			if (area.right() < levelBox.left() || area.bottom() < levelBox.top() || area.left() > levelBox.right() || area.top() > levelBox.bottom())
				continue;

			co_yield level;
		}
	}
}

std::generator<std::shared_ptr<Level>> Map::getAllLevels() const noexcept
{
	for (const auto& [levelName, levelPtr] : levelsByName)
	{
		if (auto level = getLevelPtr(levelName, levelPtr); level != nullptr)
			co_yield level;
	}
}

//----------------------------

std::shared_ptr<Level> Map::getLevelPtr(std::string_view levelName, std::weak_ptr<Level> levelPtr) const
{
	if (levelName.empty())
		return nullptr;
	if (auto level = levelPtr.lock(); level != nullptr)
		return level;

	// The level could not be locked so it was probably deleted.  Refresh it from the server.
	auto server = BabyDI::Get<Server>();
	if (auto level = server->getLevel(levelName); level != nullptr)
	{
		// Update our stored level pointers.
		if (auto it = levelsByName.find(levelName); it != levelsByName.end())
			it->second = level;
		if (auto index = level->mapPosition.x() + level->mapPosition.y() * size.width(); index < levelsByPosition.size())
			levelsByPosition[index] = level;

		return level;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
