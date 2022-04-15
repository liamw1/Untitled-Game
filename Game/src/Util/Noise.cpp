#include "GMpch.h"
#include "Noise.h"

// Operators for element-wise vector-vector multiplication
constexpr Vec2 operator*(Vec2 v, Vec2 u) { return { v.x * u.x, v.y * u.y }; }
constexpr Vec3 operator*(Vec3 v, Vec3 u) { return { v.x * u.x, v.y * u.y, v.z * u.z }; }
constexpr Vec4 operator*(Vec4 v, Vec4 u) { return { v.x * u.x, v.y * u.y, v.z * u.z, v.w * u.w }; }

/*
  Gives the fractional part of x
*/
static length_t fract(length_t x) { return x - floor(x); }

Vec2 hash(const Vec2& v)
{
  Vec2 p(glm::dot(v, Vec2(127.1, 311.7)), glm::dot(v, Vec2(269.5, 183.3)));
  return 2 * glm::fract(43758.5453123 * sin(p)) - Vec2(1.0);
}

Noise::SurfaceData::SurfaceData(length_t surfaceHeight, Block::Type blockType)
  : m_Height(surfaceHeight), m_Composition()
{
  m_Composition[0] = { blockType, 1.0 };
}

Noise::SurfaceData Noise::SurfaceData::operator+(const SurfaceData& other) const
{
  // NOTE: This function can probably be made more efficient

  std::array<Component, 2 * s_NumTypes> combinedComposition{};
  for (int i = 0; i < s_NumTypes; ++i)
    combinedComposition[i] = m_Composition[i];

  // Add like componets together and insert new components
  for (int i = 0; i < s_NumTypes; ++i)
  {
    bool otherComponentPresent = false;
    const Component& otherComponent = other.m_Composition[i];
    for (int j = 0; j < s_NumTypes; ++j)
      if (combinedComposition[j].type == otherComponent.type)
      {
        combinedComposition[j].weight += otherComponent.weight;
        otherComponentPresent = true;
        break;
      }

    if (!otherComponentPresent)
      combinedComposition[s_NumTypes + i] = otherComponent;
  }

  std::sort(combinedComposition.begin(), combinedComposition.end(), [](Component a, Component b) { return a.weight > b.weight; });

  SurfaceData sum{};
  sum.m_Height = m_Height + other.m_Height;
  for (int i = 0; i < s_NumTypes; ++i)
    sum.m_Composition[i] = combinedComposition[i];

  return sum;
}

Noise::SurfaceData Noise::SurfaceData::operator*(float x) const
{
  EN_ASSERT(x >= 0.0, "Surface data cannot be multiplied by a negative number!");

  SurfaceData result = *this;
  result.m_Height *= x;
  for (int i = 0; i < s_NumTypes; ++i)
    result.m_Composition[i].weight *= x;

  return result;
}

std::array<int, 2> Noise::SurfaceData::getTextureIndices() const
{
  std::array<int, 2> textureIndices{};

  textureIndices[0] = static_cast<int>(Block::GetTexture(m_Composition[0].type, Block::Face::Top));
  textureIndices[1] = static_cast<int>(Block::GetTexture(m_Composition[1].type, Block::Face::Top));

  return textureIndices;
}

length_t Noise::FastSimplex2D(const Vec2& v)
{
  static constexpr length_t K1 = static_cast<length_t>(0.366025404); // (sqrt(3)-1)/2;
  static constexpr length_t K2 = static_cast<length_t>(0.211324865); // (3-sqrt(3))/6;

  Vec2 i = glm::floor(v + (v.x + v.y) * K1);
  Vec2 a = v - i + Vec2((i.x + i.y) * K2);
  length_t m = glm::step(a.y, a.x);
  Vec2 o = Vec2(m, 1.0 - m);
  Vec2 b = a - o + Vec2(K2);
  Vec2 c = a + Vec2(2 * K2 - 1);
  Vec3 h = glm::max(Vec3(0.5) - Vec3(glm::dot(a, a), glm::dot(b, b), glm::dot(c, c)), Vec3(0.0));
  Vec3 n = h * h * h * h * Vec3(glm::dot(a, hash(i)), glm::dot(b, hash(i + o)), glm::dot(c, hash(i + Vec2(1.0))));
  return glm::dot(n, Vec3(70.0));
}

Noise::SurfaceData Noise::FastTerrainNoise2D(const Vec2& pointXY)
{
  length_t octave1 = 150 * Block::Length() * glm::simplex(pointXY / 1280.0 / Block::Length());
  length_t octave2 = 25 * Block::Length() * glm::simplex(pointXY / 320.0 / Block::Length());
  length_t octave3 = 5 * Block::Length() * glm::simplex(pointXY / 100.0 / Block::Length());

  length_t surfaceHeight = octave1 + octave2 + octave3;
  Block::Type blockType = Block::Type::Grass;
  if (surfaceHeight > 50 * Block::Length() + octave2)
    blockType = Block::Type::Snow;
  else if (surfaceHeight > 30 * Block::Length() + octave3)
    blockType = Block::Type::Stone;

  return SurfaceData(surfaceHeight, blockType);
}

length_t Noise::FastTerrainNoise3D(const Vec3& position)
{
  length_t octave1 = glm::simplex(position / 128.0 / Block::Length());

  return octave1;
}