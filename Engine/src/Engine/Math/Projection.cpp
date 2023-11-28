#include "ENpch.h"
#include "Projection.h"

namespace eng::math
{
  Mat4 reversedZProjection(Angle fov, f32 aspectRatio, f32 nearClip)
  {
    // Matrix is from https://stackoverflow.com/questions/214437/opengl-fast-off-screen-rendering
    f32 f = 1.0f / std::tan(fov.radf() / 2);
    return Mat4(f / aspectRatio, 0, 0, 0,
                0, f, 0, 0,
                0, 0, 0, -1,
                0, 0, nearClip, 0);
  }
}
