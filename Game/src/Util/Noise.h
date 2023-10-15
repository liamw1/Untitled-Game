#pragma once

namespace noise
{
  template<int N>
  class OctaveNoiseData
  {
  public:
    OctaveNoiseData() = default;
    OctaveNoiseData(const std::array<length_t, N>& noiseData)
      : m_Octaves(noiseData) {}

    length_t operator[](int octave) const { return m_Octaves[octave]; }
    length_t& operator[](int octave) { return m_Octaves[octave]; }

    length_t sum() const
    {
      length_t result = 0.0;
      for (length_t x : m_Octaves)
        result += x;
      return result;
    }

    static int Levels() { return N; }

  private:
    std::array<length_t, N> m_Octaves;
  };

  /*
    Faster implementation of 2D noise.
    Gives value of noise only, without derivatives.

    NOTE: Currently, glm::simplex is faster if only the
          value of noise is desired.  This can be changed 
          with a more efficient hash function.
  */
  length_t fastSimplex2D(const eng::math::Vec2& v);

  length_t fastTerrainNoise3D(const eng::math::Vec3& position);

  length_t simplexNoise2D(const eng::math::Vec2& pointXY);

  template<int N>
  OctaveNoiseData<N> octaveNoise2D(const eng::math::Vec2& pointXY, length_t lowestFrequency, float lacunarity)
  {
    std::array<length_t, N> octaves{};

    length_t frequency = lowestFrequency;
    for (int i = 0; i < N; ++i)
    {
      octaves[i] = simplexNoise2D(frequency * pointXY);
      frequency *= lacunarity;
    }

    return OctaveNoiseData<N>(octaves);
  }
}