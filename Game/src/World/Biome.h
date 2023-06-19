#pragma once
#include "Indexing.h"
#include "Chunk.h"
#include "Block/Block.h"
#include "Util/Noise.h"
#include "Util/MultiDimArrays.h"

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
  virtual void fillColumn(ArraySection<Block::Type, Chunk::Size()> column, length_t chunkFloor, length_t elevation) const = 0;

  static void Initialize();
  static const Biome* Get(Type biome);

  static constexpr int LocalElevationOctaves() { return c_LocalElevationOctaves; }
  static constexpr int Count() { return c_BiomeCount; }

protected:
  static length_t CalculateOctaveNoise(const NoiseSamples& noiseSamples, length_t largestAmplitude, float persistence);
  static void StandardColumnFill(ArraySection<Block::Type, Chunk::Size()> column, length_t chunkFloor, length_t elevation, Block::Type surfaceType, int surfaceDepth, Block::Type soilType, int soilDepth);

private:
  static constexpr int c_BiomeCount = static_cast<int>(Type::End) - static_cast<int>(Type::Begin);

  static inline int s_BiomesInitialized = 0;
  static inline std::array<std::unique_ptr<Biome>, c_BiomeCount> s_Biomes;
};