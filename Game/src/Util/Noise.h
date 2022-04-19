#pragma once
#include "Block/Block.h"

namespace Noise
{
  class SurfaceData
  {
  public:
    SurfaceData() = default;
    SurfaceData(length_t surfaceHeight, Block::Type blockType);

    SurfaceData operator+(const SurfaceData& other) const;
    SurfaceData operator*(float x) const;

    length_t getHeight() const { return m_Height; }

    Block::Type getPrimaryBlockType() const { return m_Composition[0].type; }

    std::array<int, 2> getTextureIndices() const;
    Float2 getTextureWeights() const { return { m_Composition[0].weight, m_Composition[1].weight }; }

  private:
    struct Component
    {
      Block::Type type = Block::Type::Air;
      float weight = 0.0;
    };
    static constexpr int s_NumTypes = 4;

    length_t m_Height = 0.0;
    std::array<Component, s_NumTypes> m_Composition{};
  };

  /*
    Faster implementation of 2D noise.
    Gives value of noise only, without derivatives.

    NOTE: Currently, glm::simplex is faster if only
          value of noise is desired.  This can be changed 
          with a more efficient hash function.
  */
  length_t FastSimplex2D(const Vec2& v);

  // NOTE: Replace these with more robust biome/terrain system
  SurfaceData FastTerrainNoise2D(const Vec2& pointXY);
  length_t FastTerrainNoise3D(const Vec3& position);
}

inline Noise::SurfaceData operator*(length_t x, const Noise::SurfaceData& other) { return other * static_cast<float>(x); }