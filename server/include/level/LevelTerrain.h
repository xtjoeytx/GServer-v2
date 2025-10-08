#ifndef LEVELTERRAIN_H
#define LEVELTERRAIN_H

#include <cstdint>
#include <vector>

#include <level/Map.h>
#include <utilities/Extents.h>
#include <utilities/Random.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct LevelTerrain
{
	uint32_t levelSeed = 0;
	double levelHeight = 4.0;
	double levelChaos = 0.6;
	std::vector<double> heightmap;
	std::vector<double> levelHeightOverrides;
};

struct LevelTerrainWorker
{
	double levelHeight = 4.0;
	double levelChaos = 0.6;
	DelphiRandomDeviceReal random;
	std::vector<double>* heightmap = nullptr;
	std::vector<double>* levelHeightOverrides = nullptr;
};

void generateTerrain(LevelTerrain& levelTerrain, const MapTerrain& mapTerrain, const Position<uint8_t>& mapPosition, const Dimension<uint8_t>& gridDimension);
void floodFillHeights(LevelTerrainWorker& terrain, uint32_t rowWidth, double chaos, double height, size_t bottom, size_t right, size_t top, size_t left);
void floodFillQuadrant(LevelTerrainWorker& terrain, uint32_t rowWidth, double chaos, double height, size_t bottom, size_t right, size_t top, size_t left);

void applyHeightOverrides(LevelTerrain& levelTerrain);
void applyHeightOverrideOnRow(LevelTerrain& levelTerrain, double deltaHeight, size_t tileX, size_t tileY);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELTERRAIN_H
