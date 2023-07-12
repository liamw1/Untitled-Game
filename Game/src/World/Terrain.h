#pragma once
#include "Chunk.h"
#include "Biome.h"

namespace Terrain
{
  class CompoundSurfaceData
  {
  public:
    CompoundSurfaceData();
    CompoundSurfaceData(length_t surfaceElevation, Block::Type blockType);

    CompoundSurfaceData operator+(const CompoundSurfaceData& other) const;
    CompoundSurfaceData operator*(float x) const;

    length_t getElevation() const;
    Block::Type getPrimaryBlockType() const;

    std::array<int, 2> getTextureIndices() const;
    Float2 getTextureWeights() const;

  private:
    length_t m_Elevation;
    Block::CompoundType m_Components;
  };

  inline CompoundSurfaceData operator*(length_t x, const CompoundSurfaceData& other) { return other * static_cast<float>(x); }

  struct SurfaceInfo
  {
    length_t elevation;
    Block::Type blockType;

    operator CompoundSurfaceData() const { return CompoundSurfaceData(elevation, blockType); }
  };

  Chunk GenerateNew(const GlobalIndex& chunkIndex);
  Chunk GenerateEmpty(const GlobalIndex& chunkIndex);
}