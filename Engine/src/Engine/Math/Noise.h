#pragma once
#include "Vec.h"

namespace eng::math
{
  /*
    A 2D noise function with the specified octaves and parameters.
    \returns A value whose absolute value is bounded by A[d^0 + d^1 + ... + d^n],
             where A is firstAmplitude, d is amplitudeDecay, and n is octaveCount - 1.
  */
  length_t octaveNoise(const Vec2& pointXY, i32 octaveCount, length_t firstAmplitude, f32 amplitudeDecay, length_t firstScale, f32 scaleDecay);

  /*
    A normalzed version of the 2D octave noise function
    \returns A value whose absolute value is bounded by 1.
  */
  length_t normalizedOctaveNoise(const Vec2& pointXY, i32 octaveCount, f32 amplitudeDecay, length_t firstScale, f32 scaleDecay);
}