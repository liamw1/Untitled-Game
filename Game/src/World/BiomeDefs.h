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
};