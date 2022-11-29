#pragma once
#include "Block/CompoundBlockType.h"

struct Biome
{
  length_t averageElevation;
  length_t elevationAmplitude;
  length_t elevationScale;
  float elevationPersistence;
  float elevationLacunarity;

  float averageTemperature;
  float localTemperatureVariation;
  length_t localTemperatureVariationScale;

  length_t averageSurfaceDepth;
  length_t averageSoilDepth;

  Block::CompoundType surfaceType;
  Block::CompoundType surfaceType_High;
  Block::CompoundType surfaceType_Cold;
  Block::CompoundType soilType;

  length_t highThreshold;
  float coldThreshold;

  constexpr length_t minElevation() const
  {
    length_t totalAmplitude = maxElevation() - averageElevation;
    return averageElevation - totalAmplitude;
  }

  constexpr length_t maxElevation() const
  {
    length_t totalAmplitude = 0.0;
    length_t octaveAmplitude = elevationAmplitude;
    for (int i = 0; i < s_Octaves; ++i)
    {
      totalAmplitude += octaveAmplitude;
      octaveAmplitude *= elevationPersistence;
    }

    return averageElevation + totalAmplitude;
  }

  Biome operator+(const Biome& other) const;
  Biome operator*(float x) const;

  Biome& operator+=(const Biome& other);
  Biome& operator*=(float x);

public:
  enum class Type
  {
    Default = 0,
    GrassField = 1,
    Desert = 2
  };

  static const Biome& Get(Type type);

  static constexpr int NumOctaves() { return s_Octaves; }

private:
  static constexpr int s_Octaves = 4;
};