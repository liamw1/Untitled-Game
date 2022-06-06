#pragma once
#include "Biome.h"

static constexpr Biome s_DefaultBiome =
{
  .averageElevation = 0 * Block::Length(),
  .elevationAmplitude = 150 * Block::Length(),
  .elevationScale = 1280 * Block::Length(),
  .elevationPersistence = 1.0f / 6,
  .elevationLacunarity = 4.0f,

  .averageTemperature = 20.0f,
  .localTemperatureVariation = 3.0f,
  .localTemperatureVariationScale = 300 * Block::Length(),

  .averageSurfaceDepth = 1.0,
  .averageSoilDepth = 5.0,

  .surfaceType = { Block::Type::Grass },
  .surfaceType_High = { Block::Type::Stone },
  .surfaceType_Cold = { Block::Type::Snow },
  .soilType = { Block::Type::Dirt },

  .highThreshold = 60 * Block::Length(),
  .coldThreshold = 0.0f
};

static constexpr Biome s_GrassField =
{
  .averageElevation = 10 * Block::Length(),
  .elevationAmplitude = 10 * Block::Length(),
  .elevationScale = 500 * Block::Length(),
  .elevationPersistence = 1.0f / 4,
  .elevationLacunarity = 2.0f,

  .averageTemperature = 22.0f,
  .localTemperatureVariation = 2.0f,
  .localTemperatureVariationScale = 100 * Block::Length(),

  .averageSurfaceDepth = 1.0,
  .averageSoilDepth = 4.0,

  .surfaceType = { Block::Type::Grass },
  .surfaceType_High = { Block::Type::Dirt },
  .surfaceType_Cold = { Block::Type::Snow },
  .soilType = { Block::Type::Dirt },

  .highThreshold = 50 * Block::Length(),
  .coldThreshold = 0.0f
};

static constexpr Biome s_Desert =
{
  .averageElevation = 20 * Block::Length(),
  .elevationAmplitude = 30 * Block::Length(),
  .elevationScale = 500 * Block::Length(),
  .elevationPersistence = 1.0f / 5,
  .elevationLacunarity = 3.0f,

  .averageTemperature = 45.0f,
  .localTemperatureVariation = 4.0f,
  .localTemperatureVariationScale = 250 * Block::Length(),

  .averageSurfaceDepth = 12.0,
  .averageSoilDepth = 3.0,

  .surfaceType = { Block::Type::Sand },
  .surfaceType_High = { Block::Type::Sand },
  .surfaceType_Cold = { Block::Type::Stone },
  .soilType = { Block::Type::Dirt },

  .highThreshold = 60 * Block::Length(),
  .coldThreshold = 0.0f
};