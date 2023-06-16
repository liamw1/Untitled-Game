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
    Float2 getTextureWeights() const { return { m_Components[0].weight, m_Components[1].weight }; }

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

  Array3D<Block::Type, Chunk::Size()> GenerateNew(const GlobalIndex& chunkIndex);
  Array3D<Block::Type, Chunk::Size()> GenerateEmpty(const GlobalIndex& chunkIndex);

  void Clean(int unloadDistance);
}