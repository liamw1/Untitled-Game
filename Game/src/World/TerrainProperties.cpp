#include "GMpch.h"
#include "TerrainProperties.h"

namespace terrain
{
  length_t elevationProperty(const eng::math::Vec2& pointXY)
  {
    return eng::math::normalizedOctaveNoise(pointXY, 6, 0.3f, 10000_m, 0.5f);
  }
}