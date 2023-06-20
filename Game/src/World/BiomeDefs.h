#pragma once
#include "Biome.h"

class DefaultBiome : public Biome
{
public:
  Block::Type primarySurfaceType() const override { return Block::Type::Snow; }

  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    static constexpr length_t largestElevationAmplitude = 150 * Block::Length();
    static constexpr float elevationPersistence = 1.0f / 3;

    length_t h = CalculateOctaveNoise(noiseSamples, largestElevationAmplitude, elevationPersistence);
    return std::sqrt(10 + h*h);
  }

  void fillColumn(ArraySection<Block::Type, Chunk::Size()> column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, Block::Type::Snow, 3, Block::Type::Dirt, 3);
  }
};

class GrassFieldsBiome : public Biome
{
public:
  Block::Type primarySurfaceType() const override { return Block::Type::Grass; }

  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    static constexpr length_t largestElevationAmplitude = 10 * Block::Length();
    static constexpr float elevationPersistence = 1.0f / 5;
    return CalculateOctaveNoise(noiseSamples, largestElevationAmplitude, elevationPersistence);
  }

  void fillColumn(ArraySection<Block::Type, Chunk::Size()> column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, Block::Type::Grass, 1, Block::Type::Dirt, 4);
  }
};

class DesertBoime : public Biome
{
public:
  Block::Type primarySurfaceType() const override { return Block::Type::Sand; }

  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    static constexpr length_t largestElevationAmplitude = 30 * Block::Length();
    static constexpr float elevationPersistence = 1.0f / 4;
    return CalculateOctaveNoise(noiseSamples, largestElevationAmplitude, elevationPersistence);
  }

  void fillColumn(ArraySection<Block::Type, Chunk::Size()> column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, Block::Type::Sand, 1, Block::Type::Dirt, 3);
  }
};

class FlatBiome : public Biome
{
public:
  Block::Type primarySurfaceType() const override { return Block::Type::Stone; }

  length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const override
  {
    return 0_m;
  }

  void fillColumn(ArraySection<Block::Type, Chunk::Size()> column, length_t chunkFloor, length_t elevation) const override
  {
    StandardColumnFill(column, chunkFloor, elevation, Block::Type::Stone, 1, Block::Type::Dirt, 5);
  }
};