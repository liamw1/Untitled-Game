#pragma once
#include "Block/Block.h"
#include "World/Indexing.h"
#include "Util/Noise.h"

class Biome
{
protected:
  static constexpr int c_LocalElevationOctaves = 6;

public:
  using NoiseSamples = Noise::OctaveNoiseData<c_LocalElevationOctaves>;

  enum class Type
  {
    Default = 0,
    GrassField,
    Desert,
    Flat,

    Null,
    Begin = 0, End = Null
  };

  Biome()
  {
    s_BiomesInitialized++;
  }

  virtual Block::Type primarySurfaceType() const = 0;
  virtual length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const = 0;

  static void Initialize();
  static const Biome* Get(Type biome);

  static constexpr int LocalElevationOctaves() { return c_LocalElevationOctaves; }
  static constexpr int Count() { return c_BiomeCount; }

protected:
  static length_t CalculateOctaveNoise(const NoiseSamples& noiseSamples, length_t largestAmplitude, float persistence);

private:
  static constexpr int c_BiomeCount = static_cast<int>(Type::End) - static_cast<int>(Type::Begin);

  static int s_BiomesInitialized;
  static std::array<Unique<Biome>, c_BiomeCount> s_Biomes;
};