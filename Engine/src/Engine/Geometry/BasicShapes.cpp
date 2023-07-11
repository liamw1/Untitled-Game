#include "ENpch.h"
#include "BasicShapes.h"

namespace Engine::Geometry
{
  Sphere::Sphere()
    : m_Center(0), m_Radius(0) {}
  Sphere::Sphere(const Vec3& center, length_t radius)
    : m_Center(center), m_Radius(radius) {}

  length_t Sphere::radius() { return m_Radius; }
  const Vec3& Sphere::center() { return m_Center; }



  Box::Box()
    : m_MinCorner(0), m_MaxCorner(0) {}
  Box::Box(const Vec3& minCorner, const Vec3& maxCorner)
    : m_MinCorner(minCorner), m_MaxCorner(maxCorner) {}

  const Vec3& Box::minCorner() { return m_MinCorner; }
  const Vec3& Box::maxCorner() { return m_MaxCorner; }
}
