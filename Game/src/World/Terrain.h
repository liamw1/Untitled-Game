#pragma once
#include "Chunk.h"
#include "Biome.h"
#include "Util/CompoundType.h"

namespace terrain
{
  class CompoundSurfaceData
  {
    length_t m_Elevation;
    CompoundType<block::Type, 4> m_Components;

  public:
    CompoundSurfaceData();
    CompoundSurfaceData(length_t surfaceElevation, block::ID blockType);

    CompoundSurfaceData operator+(const CompoundSurfaceData& other) const;
    CompoundSurfaceData operator*(f32 x) const;

    length_t getElevation() const;
    block::Type getPrimaryBlockType() const;

    std::array<i32, 2> getTextureIndices() const;
    eng::math::Float2 getTextureWeights() const;
  };

  inline CompoundSurfaceData operator*(length_t x, const CompoundSurfaceData& other) { return other * eng::arithmeticCastUnchecked<f32>(x); }

  struct SurfaceInfo
  {
    length_t elevation;
    block::ID blockType;

    operator CompoundSurfaceData() const { return CompoundSurfaceData(elevation, blockType); }
  };

  std::shared_ptr<Chunk> generateNew(const GlobalIndex& chunkIndex);
  std::shared_ptr<Chunk> generateEmpty(const GlobalIndex& chunkIndex);
}