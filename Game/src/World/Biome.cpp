#include "GMpch.h"
#include "Biome.h"
#include "BiomeDefs.h"

Biome Biome::operator+(const Biome& other) const
{
  Biome sum = *this;
  return sum += other;
}

Biome Biome::operator*(float x) const
{
  Biome prod = *this;
  return prod *= x;
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
