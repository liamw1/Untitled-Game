#pragma once
#include "Angle.h"
#include "Vec.h"

namespace eng::math
{
  /*
    \returns A perspective projection for a reversed z-buffer.
  */
  Mat4 reversedZProjection(Angle fov, f32 aspectRatio, f32 nearClip);
}