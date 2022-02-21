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

/*
  Modulo 289, optimizes to code without divisions
*/
static length_t mod289(length_t x) { return x - 289 * floor(x / 289); }
static Vec3 mod289(const Vec3& v) { return v - 289 * glm::floor(v / 289); }
static Vec4 mod289(const Vec4& v) { return v - 289 * glm::floor(v / 289); }

/*
  Permutation polynomial(ring size 289 = 17 ^ 2)
*/
static length_t permute(length_t x) { return mod289(x * ((34 * x) + static_cast<length_t>(10.0))); }
static Vec4 permute(const Vec4& v) { return mod289(v * ((34 * v) + Vec4(10.0))); }

Vec4 taylorInvSqrt(const Vec4& r) 
{ 
  static constexpr length_t c1 = static_cast<length_t>(1.79284291400159);
  static constexpr length_t c2 = static_cast<length_t>(0.85373472095314);
  return Vec4(c1) - c2 * r; 
}

Vec2 hash(const Vec2& v)
{
  Vec2 p = Vec2(glm::dot(v, Vec2(127.1, 311.7)), glm::dot(v, Vec2(269.5, 183.3)));
  return 2 * glm::fract(43758.5453123 * sin(p)) - Vec2(1.0);
}

/*
  Hashed 2D gradients with an extra rotation.
  The constant 0.0243902439 is 1/41
*/
static Vec2 rgrad2(Vec2 v, radians rot)
{
  length_t u = permute(permute(v.x) + v.y) / 41 + rot;

#if 1
  u = 4 * fract(u) - 2;
  // (This vector could be normalized, exactly or approximately.)
  return Vec2(abs(u) - 1, abs(abs(u + 1) - 2) - 1);
#else
  u = 2 * PI * fract(u);
  return Vec2(cos(u), sin(u));
#endif
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

Vec3 Noise::Simplex2D(const Vec2& v)
{
  static constexpr radians rot = static_cast<radians>(glm::radians(0.0));

  // Offset y slightly to hide some rare artifacts
  const Vec2 pos = v + Vec2(0.0, 0.001);

  // Skew to hexagonal grid
  Vec2 uv = Vec2(pos.x + pos.y / 2, pos.y);

  Vec2 i0 = glm::floor(uv);
  Vec2 f0 = glm::fract(uv);

  // Traversal order
  Vec2 i1 = (f0.x > f0.y) ? Vec2(1.0, 0.0) : Vec2(0.0, 1.0);

  // Unskewed grid points in (x, y) space
  Vec2 p0 = Vec2(i0.x - i0.y / 2, i0.y);
  Vec2 p1 = Vec2(p0.x + i1.x - i1.y / 2, p0.y + i1.y);
  Vec2 p2 = Vec2(p0.x + 0.5, p0.y + 1.0);

  // Integer grid point indices in (u, v) space
  i1 = i0 + i1;
  Vec2 i2 = i0 + Vec2(1.0);

  // Vectors in unskewed (x, y) coordinates from each of the
  // simplex corners to the evaluation point
  Vec2 d0 = pos - p0;
  Vec2 d1 = pos - p1;
  Vec2 d2 = pos - p2;

  Vec3 x = Vec3(p0.x, p1.x, p2.x);
  Vec3 y = Vec3(p0.y, p1.y, p2.y);
  Vec3 iuw = x + y / 2;
  Vec3 ivw = y;

  // Avoid precision issues in permuation
  iuw = mod289(iuw);
  ivw = mod289(ivw);

  // Create gradients from indices
  Vec2 g0 = rgrad2(Vec2(iuw.x, ivw.x), rot);
  Vec2 g1 = rgrad2(Vec2(iuw.y, ivw.y), rot);
  Vec2 g2 = rgrad2(Vec2(iuw.z, ivw.z), rot);

  // Gradients dot vectors to corresponding corners
  // (The derivatives of this are simply the gradients)
  Vec3 w = Vec3(glm::dot(g0, d0), glm::dot(g1, d1), glm::dot(g2, d2));

  // Radial weights from corners
  // 0.8 is the square of 2/sqrt(5), the distance from
  // a grid point to the nearest simplex boundary
  Vec3 t = Vec3(static_cast<length_t>(0.8)) - Vec3(glm::dot(d0, d0), glm::dot(d1, d1), glm::dot(d2, d2));

  // Partial derivatives for analytical gradient computation
  Vec3 dtdx = -2 * Vec3(d0.x, d1.x, d2.x);
  Vec3 dtdy = -2 * Vec3(d0.y, d1.y, d2.y);

  // Set influence of each surflet to zero outside radius sqrt(0.8)
  if (t.x < 0.0)
  {
    dtdx.x = 0.0;
    dtdy.x = 0.0;
    t.x = 0.0;
  }
  if (t.y < 0.0)
  {
    dtdx.y = 0.0;
    dtdy.y = 0.0;
    t.y = 0.0;
  }
  if (t.z < 0.0)
  {
    dtdx.z = 0.0;
    dtdy.z = 0.0;
    t.z = 0.0;
  }

  // Fourth power of t (and third power for derivative)
  Vec3 t2 = t * t;
  Vec3 t4 = t2 * t2;
  Vec3 t3 = t2 * t;

  // Final noise value is:
  // sum of ((radial weights) times (gradient dot vector from corner))
  length_t n = dot(t4, w);

  // Final analytical derivative (gradient of a sum of scalar products)
  Vec2 dt0 = 4 * t3.x * Vec2(dtdx.x, dtdy.x);
  Vec2 dt1 = 4 * t3.y * Vec2(dtdx.y, dtdy.y);
  Vec2 dt2 = 4 * t3.z * Vec2(dtdx.z, dtdy.z);
  Vec2 dn0 = t4.x * g0 + dt0 * w.x;
  Vec2 dn1 = t4.y * g1 + dt1 * w.y;
  Vec2 dn2 = t4.z * g2 + dt2 * w.z;

  return 11 * Vec3(dn0 + dn1 + dn2, n);
}

Vec4 Noise::Simplex3D(const Vec3& v)
{
  static constexpr Vec2 C = Vec2(1.0 / 6.0, 1.0 / 3.0);

  // First corner
  Vec3 i = glm::floor(v + glm::dot(v, Vec3(C.y)));
  Vec3 x0 = v - i + glm::dot(i, Vec3(C.x));

  // Other corners
  Vec3 g = glm::step(Vec3(x0.y, x0.z, x0.x), x0);
  Vec3 l = Vec3(1.0) - g;
  Vec3 i1 = glm::min(g, Vec3(l.z, l.x, l.y));
  Vec3 i2 = glm::max(g, Vec3(l.z, l.x, l.y));

  Vec3 x1 = x0 - i1 + C.x;
  Vec3 x2 = x0 - i2 + C.y;
  Vec3 x3 = x0 - Vec3(0.5);

  // Permutations
  i = mod289(i); // Avoid truncation effects in permutation
  Vec4 p = permute(permute(permute(i.z + Vec4(0.0, i1.z, i2.z, 1.0))
                                 + i.y + Vec4(0.0, i1.y, i2.y, 1.0))
                                 + i.x + Vec4(0.0, i1.x, i2.x, 1.0));

  // Gradients: 7x7 points over a square, mapped onto an octahedron.
  // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  Vec4 j = p - 49 * glm::floor(p / 49);  // mod(p,7*7)

  Vec4 x_ = glm::floor(j / 7);
  Vec4 y_ = glm::floor(j - 7 * x_);

  Vec4 x = (2 * x_ + Vec4(0.5)) / 7 - Vec4(1.0);
  Vec4 y = (2 * y_ + Vec4(0.5)) / 7 - Vec4(1.0);

  Vec4 h = Vec4(1.0) - glm::abs(x) - glm::abs(y);

  Vec4 b0 = Vec4(x.x, x.y, y.x, y.y);
  Vec4 b1 = Vec4(x.z, x.w, y.z, y.w);

  Vec4 s0 = 2 * glm::floor(b0) + Vec4(1.0);
  Vec4 s1 = 2 * glm::floor(b1) + Vec4(1.0);
  Vec4 sh = -glm::step(h, Vec4(0.0));

  Vec4 a0 = Vec4(b0.x, b0.z, b0.y, b0.w) + Vec4(s0.x * sh.x, s0.z * sh.x, s0.y * sh.y, s0.w * sh.y);
  Vec4 a1 = Vec4(b1.x, b1.z, b1.y, b1.w) + Vec4(s1.x * sh.z, s1.z * sh.z, s1.y * sh.w, s1.w * sh.w);

  Vec3 g0 = Vec3(a0.x, a0.y, h.x);
  Vec3 g1 = Vec3(a0.z, a0.w, h.y);
  Vec3 g2 = Vec3(a1.x, a1.y, h.z);
  Vec3 g3 = Vec3(a1.z, a1.w, h.w);

  // Normalize gradients
  Vec4 norm = taylorInvSqrt(Vec4(glm::dot(g0, g0), glm::dot(g1, g1), glm::dot(g2, g2), glm::dot(g3, g3)));
  g0 *= norm.x;
  g1 *= norm.y;
  g2 *= norm.z;
  g3 *= norm.w;

  // Compute noise and gradient at P
  Vec4 m = glm::max(Vec4(static_cast<length_t>(0.6)) - Vec4(glm::dot(x0, x0), glm::dot(x1, x1), glm::dot(x2, x2), glm::dot(x3, x3)), Vec4(0.0));
  Vec4 m2 = m * m;
  Vec4 m3 = m2 * m;
  Vec4 m4 = m2 * m2;
  Vec3 grad =
    -6 * m3.x * x0 * glm::dot(x0, g0) + m4.x * g0 +
    -6 * m3.y * x1 * glm::dot(x1, g1) + m4.y * g1 +
    -6 * m3.z * x2 * glm::dot(x2, g2) + m4.z * g2 +
    -6 * m3.w * x3 * glm::dot(x3, g3) + m4.w * g3;
  Vec4 px = Vec4(glm::dot(x0, g0), glm::dot(x1, g1), glm::dot(x2, g2), glm::dot(x3, g3));
  return 42 * Vec4(grad, dot(m4, px));
}

length_t Noise::FastTerrainNoise2D(const Vec2& pointXY)
{
  length_t octave1 = 150 * Block::Length() * glm::simplex(pointXY / 1280.0 / Block::Length());
  length_t octave2 = 50 * Block::Length() * glm::simplex(pointXY / 320.0 / Block::Length());
  length_t octave3 = 5 * Block::Length() * glm::simplex(pointXY / 40.0 / Block::Length());

  return octave1 + octave2 + octave3;
}

Vec4 Noise::TerrainNoise2D(const Vec2& pointXY)
{
  Vec3 octave1 = 150 * Block::Length() * Simplex2D(pointXY / 1280.0 / Block::Length());
  Vec3 octave2 = 50 * Block::Length() * Simplex2D(pointXY / 320.0 / Block::Length());
  Vec3 octave3 = 5 * Block::Length() * Simplex2D(pointXY / 40.0 / Block::Length());

  length_t value = octave1.z + octave2.z + octave3.z;
  Vec2 gradient = (Vec2(octave1) / 1280.0 + Vec2(octave2) / 320.0 + Vec2(octave3) / 40.0) / Block::Length();
  Vec3 normal = glm::normalize(Vec3(-gradient, 1));

  // return Vec4(0.0, 0.0, 1.0, -15.26246);
  return Vec4(normal, value);
}

Vec4 Noise::TerrainNoise3D(const Vec3& position)
{
  Vec4 octave1 = 150 * Block::Length() * Simplex3D(position / 1280.0 / Block::Length());

  return octave1;
}
