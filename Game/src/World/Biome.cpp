#include "GMpch.h"
#include "Biome.h"
#include "BiomeDefs.h"

Biome::Biome()
{
  s_BiomesInitialized++;
}

void Biome::Initialize()
{
  s_Biomes[static_cast<int>(Biome::Type::Default)] = std::make_unique<DefaultBiome>();
  s_Biomes[static_cast<int>(Biome::Type::GrassField)] = std::make_unique<GrassFieldsBiome>();
  s_Biomes[static_cast<int>(Biome::Type::Desert)] = std::make_unique<DesertBoime>();
  s_Biomes[static_cast<int>(Biome::Type::SuperFlat)] = std::make_unique<SuperFlatBiome>();

  if (s_BiomesInitialized != Biome::Count())
    EN_ERROR("{0} of {1} biomes have not been initialized!", Biome::Count() - s_BiomesInitialized, Biome::Count());
}

const Biome* Biome::Get(Type biome)
{
  int biomeIndex = static_cast<int>(biome);
  if (biomeIndex < 0)
    EN_INFO("Something is wrong!");
  EN_ASSERT(biomeIndex >= 0 && biomeIndex < Biome::Count(), "Invalid biome type!");
  return s_Biomes[biomeIndex].get();
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

void Biome::StandardColumnFill(BlockArrayBox<Block::Type>::Strip column, length_t chunkFloor, length_t elevation, Block::Type surfaceType, int surfaceDepth, Block::Type soilType, int soilDepth)
{
  int terrainElevationIndex = static_cast<int>(std::ceil((elevation - chunkFloor) / Block::Length()));
  int waterLevelIndex = static_cast<int>(std::ceil((0 - chunkFloor) / Block::Length()));

  blockIndex_t k = 0;
  while (k < terrainElevationIndex - soilDepth - surfaceDepth && k < Chunk::Size())
  {
    column[k] = Block::ID::Stone;
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
    column[k] = Block::ID::Water;
    k++;
  }
  while (k < Chunk::Size())
  {
    column[k] = Block::ID::Air;
    k++;
  }
}
