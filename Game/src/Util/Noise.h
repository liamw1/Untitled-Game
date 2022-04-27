#pragma once
#include "Block/CompoundBlockType.h"

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

  length_t FastTerrainNoise3D(const Vec3& position);

  std::array<length_t, 4> OctaveNoise2D(const Vec2& pointXY, length_t amplitude, length_t scale, float persistence, float lacunarity);
}