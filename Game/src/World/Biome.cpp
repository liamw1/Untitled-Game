#include "GMpch.h"
#include "Biome.h"
#include "BiomeDefs.h"

static eng::EnumArray<std::unique_ptr<Biome>, Biome::Type> initializeBiomes()
{
  eng::EnumArray<std::unique_ptr<Biome>, Biome::Type> biomes;
  biomes[Biome::Type::Default] = std::make_unique<DefaultBiome>();
  biomes[Biome::Type::GrassField] = std::make_unique<GrassFieldsBiome>();
  biomes[Biome::Type::Desert] = std::make_unique<DesertBoime>();
  biomes[Biome::Type::SuperFlat] = std::make_unique<SuperFlatBiome>();
  return biomes;
}



eng::EnumArray<std::unique_ptr<Biome>, Biome::Type> Biome::s_Biomes = initializeBiomes();

const Biome* Biome::Get(Type biome)
{
  return s_Biomes[biome].get();
}

length_t Biome::CalculateOctaveNoise(const NoiseSamples& noiseSamples, length_t largestAmplitude, float persistence)
{
  length_t sum = 0_m;
  length_t amplitude = largestAmplitude;
  for (int i = 0; i < NoiseSamples::Levels(); ++i)
  {
    sum += noiseSamples[i] * amplitude;
    amplitude *= persistence;
  }
  return sum;
}

void Biome::StandardColumnFill(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation, block::Type surfaceType, int surfaceDepth, block::Type soilType, int soilDepth)
{
  int terrainElevationIndex = static_cast<int>(std::ceil((elevation - chunkFloor) / block::length()));
  int waterLevelIndex = static_cast<int>(std::ceil((0 - chunkFloor) / block::length()));

  blockIndex_t k = 0;
  while (k < terrainElevationIndex - soilDepth - surfaceDepth && k < Chunk::Size())
  {
    column[k] = block::ID::Stone;
    k++;
  }
  while (k < terrainElevationIndex - surfaceDepth && k < Chunk::Size())
  {
    column[k] = soilType;
    k++;
  }
  while (k < terrainElevationIndex && k < Chunk::Size())
  {
    column[k] = surfaceType;
    k++;
  }
  while (k < waterLevelIndex && k < Chunk::Size())
  {
    column[k] = block::ID::Water;
    k++;
  }
  while (k < Chunk::Size())
  {
    column[k] = block::ID::Air;
    k++;
  }
}
