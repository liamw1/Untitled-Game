#pragma once
#include "Block/CompoundBlockType.h"

struct Biome
{
  length_t averageElevation = 0 * Block::Length();
  length_t elevationAmplitude = 150 * Block::Length();
  length_t elevationScale = 1280 * Block::Length();
  float elevationPersistence = 1.0f / 6;
  float elevationLacunarity = 4.0f;

  length_t averageSurfaceDepth = 1.0;
  length_t averageSoilDepth = 5.0;

  Block::CompoundType primarySurfaceType = { Block::Type::Grass };
  Block::CompoundType soilType = { Block::Type::Dirt };

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

  static constexpr int NumOctaves() { return s_Octaves; }

private:
  static constexpr int s_Octaves = 4;
};