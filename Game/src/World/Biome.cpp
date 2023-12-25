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

const Biome& Biome::Get(Type biome)
{
  return *s_Biomes[biome];
}

length_t Biome::CalculateOctaveNoise(const NoiseSamples& noiseSamples, length_t largestAmplitude, f32 persistence)
{
  length_t sum = 0;
  length_t amplitude = largestAmplitude;
  for (length_t noiseValue : noiseSamples)
  {
    sum += noiseValue * amplitude;
    amplitude *= persistence;
  }
  return sum;
}

void Biome::StandardColumnFill(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation, block::Type surfaceType, i32 surfaceDepth, block::Type soilType, i32 soilDepth)
{
  i32 terrainElevationIndex = eng::arithmeticCast<i32>(std::ceil((elevation - chunkFloor) / block::length()));
  i32 waterLevelIndex = eng::arithmeticCast<i32>(std::ceil((0 - chunkFloor) / block::length()));

  blockIndex_t k = 0;
  while (k < terrainElevationIndex - soilDepth - surfaceDepth && k < param::ChunkSize())
  {
    column[k] = block::ID::Stone;
    k++;
  }
  while (k < terrainElevationIndex - surfaceDepth && k < param::ChunkSize())
  {
    column[k] = soilType;
    k++;
  }
  while (k < terrainElevationIndex && k < param::ChunkSize())
  {
    column[k] = surfaceType;
    k++;
  }
  while (k < waterLevelIndex && k < param::ChunkSize())
  {
    column[k] = block::ID::Water;
    k++;
  }
  while (k < param::ChunkSize())
  {
    column[k] = block::ID::Air;
    k++;
  }
}
