#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <level/LevelTerrain.h>
#include <level/Map.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Random.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void generateTerrain(LevelTerrain& levelTerrain, const MapTerrain& mapTerrain, const Position<uint8_t>& mapPosition, const Dimension<uint8_t>& gridDimension)
{
	constexpr size_t numberOfHeights = static_cast<size_t>(65) * 65;
	constexpr size_t topRowStart = 0;
	constexpr size_t bottomRowStart = static_cast<size_t>(65) * 64;

	const size_t column = mapPosition[0];
	const size_t row = mapPosition[1];
	const size_t gridWidth = gridDimension[0];
	[[maybe_unused]] size_t gridHeight = gridDimension[1];

	auto& heightmap = levelTerrain.heightmap;
	if (heightmap.size() != numberOfHeights)
		heightmap.resize(numberOfHeights, std::nan(""));

	// Copy in the border heights for the tile.
	for (size_t tile = 0; tile < 65; ++tile)
	{
		const auto indexTop = (column * 64) + tile + ((gridWidth * 64 + 1) * row);
		const auto indexBottom = ((row + 1) * (gridWidth * 64 + 1)) + (column * 64) + tile;
		const auto indexLeft = ((row * 64 + tile) * (gridWidth + 1)) + column;
		const auto indexRight = ((row * 64 + tile) * (gridWidth + 1)) + column + 1;

		heightmap[topRowStart + tile] = mapTerrain.gridBorderTileHeightsXAxis[indexTop];
		heightmap[bottomRowStart + tile] = mapTerrain.gridBorderTileHeightsXAxis[indexBottom];
		heightmap[tile * 65] = mapTerrain.gridBorderTileHeightsYAxis[indexLeft];
		heightmap[tile * 65 + 64] = mapTerrain.gridBorderTileHeightsYAxis[indexRight];
	}

	LevelTerrainWorker worker{
		.levelHeight = levelTerrain.levelHeight,
		.levelChaos = levelTerrain.levelChaos,
		.random = DelphiRandomDeviceReal(mapTerrain.levelSeeds[gridWidth * row + column]),
		.heightmap = &heightmap
	};

	// Flood fill the heights.
	floodFillQuadrant(worker, 65, worker.levelChaos, worker.levelHeight * worker.levelChaos, 64, 64, 0, 0);

	// Apply terrain overrides.
	if (!levelTerrain.levelHeightOverrides.empty())
		applyHeightOverrides(levelTerrain);
}

// NOLINTNEXTLINE(*-no-recursion)
void floodFillHeights(LevelTerrainWorker& terrain, const uint32_t rowWidth, const double chaos, const double height, const size_t bottom, const size_t right, const size_t top, const size_t left)
{
	auto& heightmap = *terrain.heightmap;
	if ((right - left) > 1 || (bottom - top) > 1)
	{
		const auto midpointX = (left + right) / 2;
		const auto midpointY = (top + bottom) / 2;

		const auto randomValue = terrain.random();

		const size_t index = midpointY * rowWidth + midpointX;
		heightmap[index] = ((heightmap[top * rowWidth + left] + heightmap[bottom * rowWidth + right]) / 2.0)
			+ static_cast<double>((randomValue - 0.5) * 2.0 * height);

		floodFillHeights(terrain, rowWidth, chaos, height * chaos, midpointY, midpointX, top, left);
		floodFillHeights(terrain, rowWidth, chaos, height * chaos, bottom, right, midpointY, midpointX);
	}
}

// NOLINTNEXTLINE(*-no-recursion)
void floodFillQuadrant(LevelTerrainWorker& terrain, const uint32_t rowWidth, const double chaos, const double height, const size_t bottom, const size_t right, const size_t top, const size_t left)
{
	if ((right - left) > 1 || (bottom - top) > 1)
	{
		const auto midpointX = (left + right) / 2;
		const auto midpointY = (top + bottom) / 2;

		floodFillHeights(terrain, rowWidth, chaos, height, midpointY, right, midpointY, left);					// middle row
		floodFillHeights(terrain, rowWidth, chaos, height * chaos, midpointY, midpointX, top, midpointX);		// middle column top-half
		floodFillHeights(terrain, rowWidth, chaos, height * chaos, bottom, midpointX, midpointY, midpointX);	// middle column bottom-half

		floodFillQuadrant(terrain, rowWidth, chaos, chaos * height, midpointY, midpointX, top, left);		// top-left
		floodFillQuadrant(terrain, rowWidth, chaos, chaos * height, midpointY, right, top, midpointX);		// top-right
		floodFillQuadrant(terrain, rowWidth, chaos, chaos * height, bottom, midpointX, midpointY, left);	// bottom-left
		floodFillQuadrant(terrain, rowWidth, chaos, chaos * height, bottom, right, midpointY, midpointX);	// bottom-right
	}
}

//----------------------------

void applyHeightOverrides(LevelTerrain& levelTerrain)
{
	for (int64_t y = 0; y < 9; ++y)
	{
		for (int64_t x = 0; x < 9; ++x)
		{
			const size_t tileIndex = (y * 8) * 65 + (x * 8);
			const auto overrideHeight = levelTerrain.levelHeightOverrides[y * 9 + x];
			if (!DoublesAreSame(levelTerrain.heightmap[tileIndex], overrideHeight))
			{
				const auto tileX = x * 8;
				const auto tileY = y * 8;

				// Calculate the base delta step.
				// Each row away from the origin will have a smaller base change applied to it.
				const auto deltaStep = (overrideHeight - levelTerrain.heightmap[tileIndex]) / 8;

				// Constrain the limits to the level.
				auto top = tileY;
				auto bottom = tileY;
				if (tileY >= 8) top -= 7;
				if (tileY <= (static_cast<int64_t>(64) - 8)) bottom += 7;

				// A height override affects a 15x15 area centered on the override tile.
				// Determine how much change should be applied to each row, then adjust all the tiles in that row.
				for (size_t row = top; row <= static_cast<size_t>(bottom); ++row)
				{
					const auto rowDelta = 8 - std::abs(static_cast<int64_t>(row) - tileY);
					applyHeightOverrideOnRow(levelTerrain, deltaStep * static_cast<double>(rowDelta), tileX, row);
				}
			}
		}
	}
}

void applyHeightOverrideOnRow(LevelTerrain& levelTerrain, const double deltaHeight, const size_t tileX, const size_t tileY)
{
	const auto deltaStep = deltaHeight / 8.0;

	// Tiles to the left of the origin.
	if (tileX >= 8)
	{
		for (size_t index = 1; index < 8; ++index)
			levelTerrain.heightmap[tileY * 65 + (tileX - 8) + index] += (deltaStep * static_cast<double>(index));
	}

	// The origin.
	levelTerrain.heightmap[tileY * 65 + tileX] += deltaHeight;

	// Tiles to the right of the origin.
	if (tileY < 64)
	{
		for (size_t index = 1; index < 8; ++index)
			levelTerrain.heightmap[tileY * 65 + tileX + index] += (deltaStep * static_cast<double>(8 - index));
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
