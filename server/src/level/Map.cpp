#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
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
#include <Server.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelTerrain.h>
#include <level/Map.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/Random.h>
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
	auto& fileSystem = server->getFileSystem();
	auto fullFilePath = fileSystem.find(fs::FileCategory::FILE, fileName);

	// Make sure the file exists.
	if (fullFilePath.empty())
		throw std::runtime_error("Map file not found!");

	// Stupid.
	auto& constructSize = const_cast<Dimension<uint8_t>&>(size);
	auto& constructLevels = const_cast<string_map<Position<uint8_t>>&>(levels);
	Position<uint8_t> currentPosition;

	// Load the levels.
	auto fileData = CString::loadToken(fullFilePath.string());
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
	auto& fileSystem = server->getFileSystem();
	auto fileInfo = fileSystem.infoi(fs::FileCategory::LEVEL, fileName);

	// Make sure the file exists.
	if (fileInfo == nullptr)
		return;

	// Try to open the file.
	auto file = fileInfo->openFile();
	if (file == nullptr)
		return;

	// Save for later.
	std::string mapName{ fs::getFileNameAsANSI(fileName.stem()) };

	// Stupid.
	auto& constructSize = const_cast<Dimension<uint8_t>&>(size);
	auto& constructLevels = const_cast<string_map<Position<uint8_t>>&>(levels);
	auto& constructPreload = const_cast<string_set&>(levelsToKeepInMemory);
	auto& constructTerrain = const_cast<MapTerrain&>(terrain);
	Position<uint8_t> currentPosition;

	std::string generatedLastLevel;
	std::vector<double> terrainGridHeights;

	// Load the gmap.
	while (!file->finished())
	{
		auto line = file->readLine();
		auto lineView = string::trim(line);
		if (lineView.empty() || lineView == "GRMAP001")
			continue;

		auto [key, value] = string::extractConfigParts(lineView);
		if (key.empty())
			continue;

		if (key == "WIDTH")
		{
			constructSize.width() = string::toNumber(std::string{ value });
		}
		else if (key == "HEIGHT")
		{
			constructSize.height() = string::toNumber(std::string{ value });
		}
		else if (key == "LEVELNAMES")
		{
			currentPosition.y() = 0;

			line = file->readLine();
			lineView = string::trim(line);
			while (!lineView.starts_with("LEVELNAMESEND"))
			{
				if (currentPosition.y() < constructSize.height())
				{
					auto lines = string::fromCSV(lineView);
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

				line = file->readLine();
				lineView = string::trim(line);
			}
		}
		else if (key == "MAPIMG")
		{
			const_cast<std::string&>(mapImage) = value;
		}
		else if (key == "MINIMAPIMG")
		{
			const_cast<std::string&>(miniMapImage) = value;
		}
		else if (key == "NOAUTOMAPPING")
		{
			// Clientside only.
		}
		else if (key == "LOADFULLMAP")
		{
			const_cast<bool&>(keepAllLevelsLoaded) = true;
		}
		else if (key == "LOADATSTART")
		{
			const_cast<bool&>(keepAllLevelsLoaded) = false;

			line = file->readLine();
			lineView = string::trim(line);
			while (!lineView.starts_with("LOADATSTARTEND"))
			{
				auto lines = string::fromCSV(lineView);
				for (auto& levelName : lines)
					constructPreload.emplace(string::toLower(levelName));

				line = file->readLine();
				lineView = string::trim(line);
			}
		}
		else if (key == "GENERATED")
		{
			generatedLastLevel = string::trim(value);
		}
		else if (key == "GENSEED")
		{
			constructTerrain.mapSeed = string::toNumber<uint32_t>(std::string{ value });
		}
		else if (key == "GENBASE")
		{
			constructTerrain.heightBase = string::toDouble(std::string{ value });
		}
		else if (key == "GENEVENBORDERS")
		{
			constructTerrain.evenBorders = string::equalsi(value, "true"sv);
		}
		else if (key == "GENHEIGHT")
		{
			constructTerrain.heightDeviation = string::toDouble(std::string{ value });
		}
		else if (key == "GENCHAOS")
		{
			constructTerrain.mapChaos = string::toDouble(std::string{ value });
		}
		else if (key == "LEVHEIGHT")
		{
			constructTerrain.levelHeightDeviation = string::toDouble(std::string{ value });
		}
		else if (key == "LEVCHAOS")
		{
			constructTerrain.levelChaos = string::toDouble(std::string{ value });
		}
		else if (key == "HEIGHTMAP")
		{
			line = file->readLine();
			lineView = string::trim(line);
			while (!lineView.starts_with("HEIGHTMAPEND"))
			{
				auto lines = string::fromCSV(lineView);
				for (auto& height : lines)
					terrainGridHeights.push_back(string::toDouble(height));

				line = file->readLine();
				lineView = string::trim(line);
			}
		}
		else if (key == "RANDOMSEEDS")
		{
			line = file->readLine();
			lineView = string::trim(line);
			while (!lineView.starts_with("RANDOMSEEDSEND"))
			{
				auto lines = string::fromCSV(lineView);
				for (auto& seed : lines)
					constructTerrain.levelSeeds.push_back(string::toNumber<uint32_t>(seed));

				line = file->readLine();
				lineView = string::trim(line);
			}
		}
	}

	// Size the positional storage.
	levelsByPosition.resize(static_cast<size_t>(constructSize.width()* constructSize.height()));

	// If we don't have any levels, but we do have a generated level end, automatically create the levels.
	if (constructLevels.empty() && !generatedLastLevel.empty())
	{
		auto columnDigits = std::floor(std::log(constructSize.width()) / std::log(26)) + 1;
		auto rowDigits = std::floor(std::log10(constructSize.height())) + 1;

		auto toColumnName = [](const size_t digits, size_t col) -> std::string
		{
			std::string result(digits, 'a');
			auto iter = result.rbegin();
			while (col > 0 && iter != result.rend())
			{
				auto remainder = col % 26;
				*iter = 'a' + remainder;
				col /= 26;
				++iter;
			}
			return result;
		};

		// Using the pattern of the generated level name, create all the levels.
		// prefix|column|row.nw
		// Example: mymap_a-1.nw or mymap_a1.nw

		// First, determine the separator between the prefix and the columns.
		std::string_view genLevel{ generatedLastLevel };
		std::string_view levelPrefix;
		std::string_view columnSeparator = "_"sv;
		std::string_view rowSeparator = "-"sv;

		// Generated level starts with the map name.
		if (genLevel.starts_with(fileName.stem().string()))
		{
			levelPrefix = genLevel.substr(0, fileName.stem().string().size());
			genLevel = genLevel.substr(levelPrefix.length());
		}
		// Search for a - or _ separator.
		else if (auto sepPos = genLevel.find_first_of("-_"sv); sepPos != std::string_view::npos)
		{
			levelPrefix = genLevel.substr(0, sepPos);
			genLevel = genLevel.substr(levelPrefix.length());
		}

		// If we can't figure out the generated level prefix, just use the map file name.
		if (levelPrefix.empty())
		{
			log::printLine(log::server, "** Could not determine generated level prefix for map '{}', using map name.", mapName);
			levelPrefix = mapName;
		}
		else
		{
			// Find the first alphabetic character.
			auto alphaPos = genLevel.find_first_of("abcdefghijklmnopqrstuvwxyz"sv);
			if (alphaPos != std::string_view::npos)
			{
				columnSeparator = genLevel.substr(0, alphaPos);
				genLevel = genLevel.substr(alphaPos);

				if (auto last = genLevel.find_first_not_of("abcdefghijklmnopqrstuvwxyz"sv); last != std::string_view::npos)
					genLevel = genLevel.substr(last);

				// Find the first numeric character.
				auto numericPos = genLevel.find_first_of("0123456789"sv);
				if (numericPos != std::string_view::npos)
					rowSeparator = genLevel.substr(0, numericPos);
			}
		}

		std::string row;
		for (size_t y = 0; y < constructSize.height(); ++y)
		{
			row = std::format("{:0{}}", y + 1, static_cast<int>(rowDigits));
			for (size_t x = 0; x < constructSize.width(); ++x)
			{
				auto levelName = std::format("{}{}{}{}{}.nw", levelPrefix, columnSeparator, toColumnName(columnDigits, x), rowSeparator, row);
				constructLevels.insert({ string::toLower(levelName), Position<uint8_t>{ static_cast<uint8_t>(x), static_cast<uint8_t>(y) } });
			}
		}
	}

	// If we have terrain heights, generate the row/column border heights.
	if (!terrainGridHeights.empty())
	{
		size_t gridWidth = constructSize.width();
		size_t gridHeight = constructSize.height();
		constructTerrain.gridBorderTileHeightsXAxis.resize((gridWidth * 64 + 1)* (gridHeight + 1));
		constructTerrain.gridBorderTileHeightsYAxis.resize((gridHeight * 64 + 1)* (gridWidth + 1));

		// Get the corner heights for the map grid.
		for (size_t column = 0; column <= gridWidth; ++column)
		{
			for (size_t row = 0; row <= gridHeight; ++row)
			{
				size_t index = (gridWidth + 1) * row + column;
				size_t indexAxisX = ((gridWidth * 64 + 1) * row) + (column * 64);
				size_t indexAxisY = (row * 64 * (gridWidth + 1)) + column;

				if (index >= terrainGridHeights.size()
					|| indexAxisX >= constructTerrain.gridBorderTileHeightsXAxis.size()
					|| indexAxisY >= constructTerrain.gridBorderTileHeightsYAxis.size())
					throw std::runtime_error("Invalid terrain height data in gmap file!");

				auto heightValue = terrainGridHeights[index];
				constructTerrain.gridBorderTileHeightsXAxis[indexAxisX] = heightValue;
				constructTerrain.gridBorderTileHeightsYAxis[indexAxisY] = heightValue;
			}
		}

		// Set our seed.
		LevelTerrainWorker worker{
			.levelHeight = constructTerrain.levelHeightDeviation,
			.levelChaos = constructTerrain.levelChaos,
			.random = DelphiRandomDeviceReal(constructTerrain.mapSeed),
			.heightmap = &constructTerrain.gridBorderTileHeightsXAxis
		};

		// Fill in the border heights for the map grid.
		for (size_t column = 0; column <= gridWidth; ++column)
		{
			for (size_t row = 0; row <= gridHeight; ++row)
			{
				if (column < gridWidth)
				{
					worker.heightmap = &constructTerrain.gridBorderTileHeightsXAxis;
					floodFillHeights(worker, gridWidth * 64 + 1, worker.levelChaos, worker.levelHeight, row, (column + 1) * 64, row, column * 64);
				}

				if (row < gridHeight)
				{
					worker.heightmap = &constructTerrain.gridBorderTileHeightsYAxis;
					floodFillHeights(worker, gridWidth + 1, worker.levelChaos, worker.levelHeight, (row + 1) * 64, column, row * 64, column);
				}
			}
		}
	}
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

void Map::setLevelLoaded(std::shared_ptr<Level> level)
{
	if (auto it = levelsByName.find(level->levelName); it != levelsByName.end())
		it->second = level;
	if (size_t index = level->mapPosition.x() + level->mapPosition.y() * size.width(); index < levelsByPosition.size())
		levelsByPosition[index] = level;
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

std::generator<std::shared_ptr<Level>> Map::getLevelsInRange(const TilePosition& position, int syncTilesX, int syncTilesY) const noexcept
{
	Position<int16_t> searchPos{ static_cast<int16_t>(position.x() / 64), static_cast<int16_t>(position.y() / 64) };
	Dimension<uint8_t> levelSyncDistance{ static_cast<uint8_t>(std::ceilf(syncTilesX / 64)), static_cast<uint8_t>(std::ceilf(syncTilesY / 64)) };
	Rectangle<int16_t, uint8_t> area{ searchPos.translate(-levelSyncDistance.width(), -levelSyncDistance.height()), levelSyncDistance * 2 };

	for (const auto& [levelName, levelPos] : levels)
	{
		if (levelPos.x() >= area.left() && levelPos.x() <= area.right() &&
			levelPos.y() >= area.top() && levelPos.y() <= area.bottom())
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

void Map::forceSetLevelLoaded(std::shared_ptr<Level> level) const noexcept
{
	if (auto it = levelsByName.find(level->levelName); it != levelsByName.end())
		it->second = level;
	if (size_t index = level->mapPosition.x() + level->mapPosition.y() * size.width(); index < levelsByPosition.size())
		levelsByPosition[index] = level;
}

std::shared_ptr<Level> Map::getLevelPtr(std::string_view levelName, std::weak_ptr<Level> levelPtr) const noexcept
{
	if (levelName.empty())
		return nullptr;
	if (auto level = levelPtr.lock(); level != nullptr)
		return level;

	// The level could not be locked, so ask the server for a stub.
	auto server = BabyDI::Get<Server>();
	if (auto level = server->stubOrGetLevel(levelName); level != nullptr)
	{
		level->setMap(server->findMap(getMapName()));
		forceSetLevelLoaded(level);
		return level;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
