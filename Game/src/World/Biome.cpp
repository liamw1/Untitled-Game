#include "GMpch.h"
#include "Biome.h"
#include "BiomeDefs.h"
#include "Chunk.h"

int Biome::s_BiomesInitialized = 0;
std::array<Unique<Biome>, Biome::Count()> Biome::s_Biomes{};

void Biome::Initialize()
{
  s_Biomes[static_cast<int>(Biome::Type::Default)] = CreateUnique<DefaultBiome>();
  s_Biomes[static_cast<int>(Biome::Type::GrassField)] = CreateUnique<GrassFieldsBiome>();
  s_Biomes[static_cast<int>(Biome::Type::Desert)] = CreateUnique<DesertBoime>();

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
