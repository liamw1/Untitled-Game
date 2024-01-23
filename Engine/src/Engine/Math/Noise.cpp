#include "ENpch.h"
#include "Noise.h"
#include "Engine/Core/Algorithm.h"

#include <glm/gtc/noise.hpp>

namespace eng::math
{
  length_t octaveNoise(const Vec2& pointXY, i32 octaveCount, length_t firstAmplitude, f32 amplitudeDecay, length_t firstScale, f32 scaleDecay)
  {
    length_t noiseValue = 0_m;
    for (i32 i = 0; i < octaveCount; ++i)
    {
      noiseValue += firstAmplitude * glm::simplex(pointXY / firstScale);
      firstAmplitude *= amplitudeDecay;
      firstScale *= scaleDecay;
    }
    return noiseValue;
  }

  length_t normalizedOctaveNoise(const Vec2& pointXY, i32 octaveCount, f32 amplitudeDecay, length_t firstScale, f32 scaleDecay)
  {
    length_t normalizedAmplitude = 1_m;
    length_t normalizationFactor = 0_m;
    for (i32 i = 0; i < octaveCount; ++i)
    {
      normalizationFactor += normalizedAmplitude;
      normalizedAmplitude *= amplitudeDecay;
    }

    return octaveNoise(pointXY, octaveCount, 1_m / normalizationFactor, amplitudeDecay, firstScale, scaleDecay);
  }
}