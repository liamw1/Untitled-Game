#include "GMpch.h"
#include "Noise.h"
#include "Block/Block.h"

#include <glm/gtc/noise.hpp>

// Operators for element-wise vector-vector multiplication
constexpr eng::math::Vec2 operator*(eng::math::Vec2 v, eng::math::Vec2 u) { return { v.x * u.x, v.y * u.y }; }
constexpr eng::math::Vec3 operator*(eng::math::Vec3 v, eng::math::Vec3 u) { return { v.x * u.x, v.y * u.y, v.z * u.z }; }
constexpr eng::math::Vec4 operator*(eng::math::Vec4 v, eng::math::Vec4 u) { return { v.x * u.x, v.y * u.y, v.z * u.z, v.w * u.w }; }

/*
  Gives the fractional part of x
*/
static length_t fract(length_t x) { return x - floor(x); }

eng::math::Vec2 hash(const eng::math::Vec2& v)
{
  eng::math::Vec2 p(glm::dot(v, eng::math::Vec2(127.1, 311.7)), glm::dot(v, eng::math::Vec2(269.5, 183.3)));
  return 2 * glm::fract(43758.5453123 * sin(p)) - eng::math::Vec2(1.0);
}

length_t noise::fastSimplex2D(const eng::math::Vec2& v)
{
  static constexpr length_t K1 = (std::numbers::sqrt3_v<length_t> - 1) / 2;
  static constexpr length_t K2 = (3 - std::numbers::sqrt3_v<length_t>) / 6;

  eng::math::Vec2 i = glm::floor(v + (v.x + v.y) * K1);
  eng::math::Vec2 a = v - i + eng::math::Vec2((i.x + i.y) * K2);
  length_t m = glm::step(a.y, a.x);
  eng::math::Vec2 o = eng::math::Vec2(m, 1.0 - m);
  eng::math::Vec2 b = a - o + eng::math::Vec2(K2);
  eng::math::Vec2 c = a + eng::math::Vec2(2 * K2 - 1);
  eng::math::Vec3 h = glm::max(eng::math::Vec3(0.5) - eng::math::Vec3(glm::dot(a, a), glm::dot(b, b), glm::dot(c, c)), eng::math::Vec3(0.0));
  eng::math::Vec3 n = h * h * h * h * eng::math::Vec3(glm::dot(a, hash(i)), glm::dot(b, hash(i + o)), glm::dot(c, hash(i + eng::math::Vec2(1.0))));
  return glm::dot(n, eng::math::Vec3(70.0));
}

length_t noise::fastTerrainNoise3D(const eng::math::Vec3& position)
{
  length_t octave1 = glm::simplex(position / 128.0 / block::length());

  return octave1;
}

length_t noise::simplexNoise2D(const eng::math::Vec2& pointXY)
{
  return glm::simplex(pointXY);
}
