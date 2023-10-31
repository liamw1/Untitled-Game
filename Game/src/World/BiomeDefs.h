#pragma once
#include "Biome.h"

class DefaultBiome : public Biome
{
public:
  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    static constexpr length_t largestElevationAmplitude = 150 * block::length();
    static constexpr f32 elevationPersistence = 1.0f / 3;

    length_t h = CalculateOctaveNoise(noiseSamples, largestElevationAmplitude, elevationPersistence);
    return std::sqrt(10 + h * h);
  }

  void fillColumn(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, block::ID::Snow, 3, block::ID::Dirt, 3);
  }
};

class GrassFieldsBiome : public Biome
{
public:
  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    static constexpr length_t largestElevationAmplitude = 10 * block::length();
    static constexpr f32 elevationPersistence = 1.0f / 5;
    return CalculateOctaveNoise(noiseSamples, largestElevationAmplitude, elevationPersistence);
  }

  void fillColumn(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, block::ID::Grass, 1, block::ID::Dirt, 4);
  }
};

class DesertBoime : public Biome
{
public:
  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    static constexpr length_t largestElevationAmplitude = 30 * block::length();
    static constexpr f32 elevationPersistence = 1.0f / 4;
    return CalculateOctaveNoise(noiseSamples, largestElevationAmplitude, elevationPersistence);
  }

  void fillColumn(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, block::ID::Sand, 1, block::ID::Dirt, 3);
  }
};

class SuperFlatBiome : public Biome
{
public:
  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    return block::length() / 2;
  }

  void fillColumn(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, block::ID::Grass, 1, block::ID::Dirt, 5);
  }
};