#pragma once

namespace Noise
{
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
  length_t FastTerrainNoise2D(const Vec2& pointXY);
  Vec4 TerrainNoise2D(const Vec2& pointXY);
  Vec4 TerrainNoise3D(const Vec3& position);
}