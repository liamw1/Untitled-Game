#pragma once
#include "Indexing.h"
#include "Chunk.h"
#include "Block/Block.h"
#include "Util/Noise.h"

class Biome
{
protected:
  static constexpr i32 c_LocalElevationOctaves = 6;

public:
  using NoiseSamples = noise::OctaveNoiseData<c_LocalElevationOctaves>;

  enum class Type
  {
    Default,
    GrassField,
    Desert,
    SuperFlat,

    Null,
    First = 0, Last = Null - 1
  };

  virtual length_t localSurfaceElevation(const NoiseSamples& noiseSamples) const = 0;
  virtual void fillColumn(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation) const = 0;

  static const Biome* Get(Type biome);

  static constexpr i32 LocalElevationOctaves() { return c_LocalElevationOctaves; }
  static constexpr i32 Count() { return static_cast<i32>(s_Biomes.size()); }

protected:
  static length_t CalculateOctaveNoise(const NoiseSamples& noiseSamples, length_t largestAmplitude, f32 persistence);
  static void StandardColumnFill(BlockArrayBox<block::Type>::Strip column, length_t chunkFloor, length_t elevation, block::Type surfaceType, i32 surfaceDepth, block::Type soilType, i32 soilDepth);

private:
  static eng::EnumArray<std::unique_ptr<Biome>, Type> s_Biomes;
};