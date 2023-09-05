#include "GMpch.h"
#include "Noise.h"
#include "Block/Block.h"
#include <glm/gtc/noise.hpp>

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

length_t Noise::FastSimplex2D(const Vec2& v)
{
  static constexpr length_t K1 = (std::numbers::sqrt3_v<length_t> - 1) / 2;
  static constexpr length_t K2 = (3 - std::numbers::sqrt3_v<length_t>) / 6;

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

length_t Noise::FastTerrainNoise3D(const Vec3& position)
{
  length_t octave1 = glm::simplex(position / 128.0 / Block::Length());

  return octave1;
}

length_t Noise::SimplexNoise2D(const Vec2& pointXY)
{
  return glm::simplex(pointXY);
}
