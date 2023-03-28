#include "GMpch.h"
#include "Biome.h"
#include "BiomeDefs.h"
#include "Chunk.h"

Biome Biome::operator+(const Biome& other) const
{
  Biome copy = *this;
  return copy += other;
}

Biome Biome::operator*(float x) const
{
  Biome copy = *this;
  return copy *= x;
}

Biome& Biome::operator+=(const Biome& other)
{
  averageElevation += other.averageElevation;
  elevationAmplitude += other.elevationAmplitude;
  elevationScale += other.elevationScale;
  elevationPersistence += other.elevationPersistence;
  elevationLacunarity += other.elevationLacunarity;

  averageTemperature += other.averageTemperature;
  localTemperatureVariation += other.localTemperatureVariation;
  localTemperatureVariationScale += other.localTemperatureVariationScale;

  averageSurfaceDepth += other.averageSurfaceDepth;
  averageSoilDepth += other.averageSoilDepth;

  surfaceType += other.surfaceType;
  surfaceType_High += other.surfaceType_High;
  surfaceType_Cold += other.surfaceType_Cold;
  soilType += other.soilType;

  highThreshold += other.highThreshold;
  coldThreshold += other.coldThreshold;

  return *this;
}

Biome& Biome::operator*=(float x)
{
  averageElevation *= x;
  elevationAmplitude *= x;
  elevationScale *= x;
  elevationPersistence *= x;
  elevationLacunarity *= x;

  averageTemperature *= x;
  localTemperatureVariation *= x;
  localTemperatureVariationScale *= x;

  averageSurfaceDepth *= x;
  averageSoilDepth *= x;

  surfaceType *= x;
  surfaceType_High *= x;
  surfaceType_Cold *= x;
  soilType *= x;

  highThreshold *= x;
  coldThreshold *= x;

  return *this;
}

const Biome& Biome::Get(Type type)
{
  switch (type)
  {
    case Biome::Type::Default:    return s_DefaultBiome;
    case Biome::Type::GrassField: return s_GrassField;
    case Biome::Type::Desert:     return s_Desert;
    default: EN_ERROR("Unknown biome type!"); return s_DefaultBiome;
  }
}

// From https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
static uint32_t hash(uint32_t n)
{
  n = ((n >> 16) ^ n) * 0x45d9f3b;
  n = ((n >> 16) ^ n) * 0x45d9f3b;
  return (n >> 16) ^ n;
}

// Returns a random float in the range [0.0, 1.0] based determinisitically on the input n
static float random(uint32_t n)
{
  return static_cast<float>(hash(n)) / std::numeric_limits<uint32_t>::max();
}

// Returns a biome type based determinisitically on the input n
static Biome::Type randomBiome(uint32_t n)
{
  static constexpr int numBiomes = static_cast<int>(Biome::Type::Last) - static_cast<int>(Biome::Type::First) + 1;
  return static_cast<Biome::Type>(hash(n) % numBiomes);
}

static std::pair<Biome::Type, Float2> getRegionVoronoiPoint(const SurfaceMapIndex& regionIndex)
{
  uint32_t key = std::hash<SurfaceMapIndex>{}(regionIndex);
  float r = 0.5f * random(key);
  float theta = 2 * Constants::PI * random(hash(key));

  Biome::Type biomeType = randomBiome(key);
  Float2 relativeLocation(r * std::cos(theta), r * std::sin(theta));
  return { biomeType, relativeLocation };
}

Biome GetBiomeData(const Vec2& surfaceLocation)
{
  static constexpr int biomeRegionSize = 16;
  static constexpr int regionRadius = 1;
  static constexpr int regionWidth = 2 * regionRadius + 1;
  SurfaceMapIndex queryRegionIndex = SurfaceMapIndex::ToIndex(surfaceLocation / Chunk::Size() / biomeRegionSize);

  std::array<std::pair<Biome::Type, Float2>, regionWidth * regionWidth> neighboringVoronoiPoints;
  for (globalIndex_t i = -regionRadius; i <= regionRadius; ++i)
    for (globalIndex_t j = -regionRadius; j <= regionRadius; ++j)
    {
      SurfaceMapIndex regionIndex = queryRegionIndex + SurfaceMapIndex(i, j);
      Float2 regionCenterRelativeToQueryRegion(i + 0.5f, j + 0.5f);

      auto [biomeType, voronoiPointPerturbation] = getRegionVoronoiPoint(regionIndex);
      neighboringVoronoiPoints[regionWidth * (i + regionRadius) + j + regionRadius] = { biomeType, regionCenterRelativeToQueryRegion + voronoiPointPerturbation };
    }

  Biome biome{};
  float sumOfWeights = 0.0;
  Float2 queryLocationRelativeToQueryRegion = surfaceLocation / Chunk::Size() / biomeRegionSize - static_cast<Vec2>(queryRegionIndex);
  for (const auto& [biomeType, location] : neighboringVoronoiPoints)
  {
    float distance = glm::distance(queryLocationRelativeToQueryRegion, location);
    float biomeWeight = std::max(0.0f, std::expf(-32 * distance * distance) - std::expf(-32));
    sumOfWeights += biomeWeight;

    biome += Biome::Get(biomeType) * biomeWeight;
  }

  return biome * (1.0f / sumOfWeights);
}