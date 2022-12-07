#pragma once
#include "Block/CompoundBlockType.h"

namespace Noise
{
  template<int N>
  class OctaveNoiseData
  {
  public:
    OctaveNoiseData() = default;

    OctaveNoiseData(const std::array<length_t, N>& noiseData)
      : m_Octaves(noiseData) {}

    length_t operator[](int octave) const { return m_Octaves[octave]; }

    length_t sum() const
    {
      length_t result = 0.0;
      for (length_t x : m_Octaves)
        result += x;
      return result;
    }

  private:
    std::array<length_t, N> m_Octaves{};
  };

  /*
    Faster implementation of 2D noise.
    Gives value of noise only, without derivatives.

    NOTE: Currently, glm::simplex is faster if only the
          value of noise is desired.  This can be changed 
          with a more efficient hash function.
  */
  length_t FastSimplex2D(const Vec2& v);

  length_t FastTerrainNoise3D(const Vec3& position);

  template<typename floatType, glm::qualifier Q>
  floatType SimplexNoise2D(const glm::vec<2, floatType, Q>& v)
  {
    return glm::simplex(v);
  }

  template<int N>
  OctaveNoiseData<N> OctaveNoise2D(const Vec2& pointXY, length_t amplitude, length_t scale, float persistence, float lacunarity)
  {
    std::array<length_t, N> octaves{};

    length_t frequency = 1_m / scale;
    for (int i = 0; i < N; ++i)
    {
      octaves[i] = amplitude * SimplexNoise2D(frequency * pointXY);
      amplitude *= persistence;
      frequency *= lacunarity;
    }

    return OctaveNoiseData<N>(octaves);
  }
}