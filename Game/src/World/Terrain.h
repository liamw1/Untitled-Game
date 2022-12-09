#pragma once
#include "Chunk.h"
#include "Biome.h"

namespace Terrain
{
  class CompoundSurfaceData
  {
  public:
    CompoundSurfaceData() = default;
    CompoundSurfaceData(length_t surfaceElevation, Block::Type blockType)
      : m_Elevation(surfaceElevation), m_Components(blockType)
    {
    }

    CompoundSurfaceData operator+(const CompoundSurfaceData& other) const;
    CompoundSurfaceData operator*(float x) const;

    length_t getElevation() const { return m_Elevation; }

    Block::Type getPrimaryBlockType() const { return m_Components.getPrimary(); }

    std::array<int, 2> getTextureIndices() const;
    Float2 getTextureWeights() const { return { m_Components.getWeight(0), m_Components.getWeight(1) }; }

  private:
    length_t m_Elevation = 0.0;
    Block::CompoundType m_Components;
  };

  inline CompoundSurfaceData operator*(length_t x, const CompoundSurfaceData& other) { return other * static_cast<float>(x); }

  struct SurfaceInfo
  {
    length_t elevation;
    Block::Type blockType;

    operator CompoundSurfaceData () const { return CompoundSurfaceData(elevation, blockType); }
  };

  Chunk GenerateNew(const GlobalIndex& chunkIndex);
  Chunk GenerateEmpty(const GlobalIndex& chunkIndex);

  void Clean(int unloadDistance);

  Noise::OctaveNoiseData<Biome::NumOctaves()> GetElevationData(const Vec2& pointXY, const Biome& biome);
  float GetTemperatureData(const Vec2& pointXY, const Biome& biome);

  SurfaceInfo GetSurfaceInfo(const Noise::OctaveNoiseData<Biome::NumOctaves()>& elevationData, float seaLevelTemperature, const Biome& biome);
}