#include "GMpch.h"
#include "Noise.h"

static constexpr bool quinticInterpolation = true;

/*
  Returns fractional part of x
*/
static length_t fract(length_t x)
{
  return x - floor(x);
}

static Vec2 hash2D(const Vec2& v)
{
  const Vec2 k = Vec2(0.3183099, 0.3678794);
  Vec2 v2 = v * k + Vec2(k.y, k.x);
  return 2 * fract(16 + fract(v2.x * v2.y * (v2.x + v2.y))) * k - Vec2(1.0);
}

Vec3 Noise::GradientNoise2D(const Vec2& v)
{
  Vec2 i = glm::floor(v);
  Vec2 f = glm::fract(v);

  Vec2 u, du{};
  if (quinticInterpolation)
  {
    u = f * f * f * (f * (6 * f - Vec2(15.0)) + Vec2(10.0));
    du = 30 * f * f * (f * (f - Vec2(2.0)) + Vec2(1.0));
  }
  else // Cubic interpolation
  {
    u = f * f * (Vec2(3.0) - 2 * f);
    du = 6 * f * (Vec2(1.0) - f);
  }

  Vec2 ga = hash2D(i + Vec2(0.0, 0.0));
  Vec2 gb = hash2D(i + Vec2(1.0, 0.0));
  Vec2 gc = hash2D(i + Vec2(0.0, 1.0));
  Vec2 gd = hash2D(i + Vec2(1.0, 1.0));

  length_t va = glm::dot(ga, f - Vec2(0.0, 0.0));
  length_t vb = glm::dot(gb, f - Vec2(1.0, 0.0));
  length_t vc = glm::dot(gc, f - Vec2(0.0, 1.0));
  length_t vd = glm::dot(gd, f - Vec2(1.0, 1.0));

  return Vec3(va + u.x * (vb - va) + u.y * (vc - va) + u.x * u.y * (va - vb - vc + vd),   // value
              ga + u.x * (gb - ga) + u.y * (gc - ga) + u.x * u.y * (ga - gb - gc + gd) +  // derivatives
              du * (Vec2(u.y, u.x) * (va - vb - vc + vd) + Vec2(vb, vc) - va));
}
