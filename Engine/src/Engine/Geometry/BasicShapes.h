#pragma once

namespace Engine::Geometry
{
  class Sphere
  {
  public:
    Sphere();
    Sphere(const Vec3& center, length_t radius);

    length_t radius();
    const Vec3& center();

  private:
    Vec3 m_Center;
    length_t m_Radius;
  };

  class Box
  {
  public:
    Box();
    Box(const Vec3& minCorner, const Vec3& maxCorner);

    const Vec3& minCorner();
    const Vec3& maxCorner();

  private:
    Vec3 m_MinCorner;
    Vec3 m_MaxCorner;
  };
}