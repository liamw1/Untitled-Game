#pragma once

namespace Noise
{
  /*
    2-D non-tiling simplex noise and analytical derivative without rotating gradients.
    The third component of the 3-element return vector is the noise value,
    and the first and second components are the x and y partial derivatives.
  */
  Vec3 Simplex2D(const Vec2& v);

  Vec4 Simplex3D(const Vec3& v);

  length_t FastSimplex2D(const Vec2& v);

  // NOTE: Replace these with more robust biome/terrain system
  length_t FastTerrainNoise2D(const Vec2& pointXY);
  Vec4 TerrainNoise2D(const Vec2& pointXY);
  Vec4 TerrainNoise3D(const Vec3& position);
}