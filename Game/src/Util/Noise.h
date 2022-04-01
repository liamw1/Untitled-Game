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
    int getPrimaryTextureIndex() const { return static_cast<int>(Block::GetTexture(m_Composition[0].type, Block::Face::Top)); }

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

  /*
    2D non-tiling simplex noise and analytical derivative without rotating gradients.
    The third component of the 3-element return vector is the noise value,
    and the first and second components are the x and y partial derivatives.
  */
  Vec3 Simplex2D(const Vec2& v);

  /*
    3D non-tiling simplex noise and analytical derivative.
    The fourth component of the 3-element return vector is the noise value,
    and the first, second, and third components are the x, y, and z partial derivatives.
  */
  Vec4 Simplex3D(const Vec3& v);

  // NOTE: Replace these with more robust biome/terrain system
  SurfaceData FastTerrainNoise2D(const Vec2& pointXY);
  Vec4 TerrainNoise2D(const Vec2& pointXY);
  Vec4 TerrainNoise3D(const Vec3& position);
}

inline Noise::SurfaceData operator*(length_t x, const Noise::SurfaceData& other) { return other * x; }