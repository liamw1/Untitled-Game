#include "ENpch.h"
#include "Camera.h"
#include "Engine/Debug/Assert.h"
#include "Engine/Math/Projection.h"

#include <glm/gtc/matrix_transform.hpp>

namespace eng
{
  Camera::Camera()
    : m_Projection(math::Mat4(1)),
      m_ProjectionType(ProjectionType::Perspective),
      m_AspectRatio(1280.0f / 720),
      m_NearClip(0.0f),
      m_FOV(80_deg),
      m_OrthographicSize(1.0f) {}

  const math::Mat4& Camera::projectionMatrix() const { return m_Projection; }
  Camera::ProjectionType Camera::projectionType() const { return m_ProjectionType; }

  f32 Camera::nearClip() const { return m_NearClip; }

  f32 Camera::aspectRatio() const { return m_AspectRatio; }
  void Camera::setAspectRatio(f32 aspectRatio)
  {
    m_AspectRatio = aspectRatio;
    recalculatePerspectiveProjection();
  }

  math::Angle Camera::fov() const
  {
    ENG_CORE_ASSERT(m_ProjectionType == ProjectionType::Perspective, "An orthographic camera does not have a FOV!");
    return m_FOV;
  }

  void Camera::setFov(math::Angle fov)
  {
    ENG_CORE_ASSERT(m_ProjectionType == ProjectionType::Perspective, "An orthographic camera does not have a FOV!");
    m_FOV = fov;
    recalculatePerspectiveProjection();
  }

  f32 Camera::orthographicSize() const
  {
    ENG_CORE_ASSERT(m_ProjectionType == ProjectionType::Orthographic, "A perspective camera does not have an orthographic size!");
    return m_OrthographicSize;
  }

  void Camera::setOrthographicView(f32 aspectRatio, f32 size, f32 nearClip)
  {
    m_AspectRatio = aspectRatio;
    m_OrthographicSize = size;
    m_NearClip = nearClip;
    m_ProjectionType = ProjectionType::Orthographic;

    recalculateOrthographicProjection();
  }

  void Camera::setPerspectiveView(f32 aspectRatio, math::Angle fov, f32 nearClip)
  {
    m_AspectRatio = aspectRatio;
    m_FOV = fov;
    m_NearClip = nearClip;
    m_ProjectionType = ProjectionType::Perspective;

    recalculatePerspectiveProjection();
  }

  void Camera::setViewportSize(u32 width, u32 height)
  {
    m_AspectRatio = height > 0 ? arithmeticUpcast<f32>(width) / height : 0.0f;
    recalculateProjection();
  }

  void Camera::recalculateProjection()
  {
    switch (m_ProjectionType)
    {
      case ProjectionType::Orthographic: recalculateOrthographicProjection(); return;
      case ProjectionType::Perspective:  recalculatePerspectiveProjection();  return;
    }
    throw CoreException("Invalid projection type!");
  }

  void Camera::recalculateOrthographicProjection()
  {
    throw CoreException("Reversed-Z orthographic projection not yet implemented!");
  }

  void Camera::recalculatePerspectiveProjection()
  {
    m_Projection = reversedZProjection(m_FOV, m_AspectRatio, m_NearClip);
  }
}