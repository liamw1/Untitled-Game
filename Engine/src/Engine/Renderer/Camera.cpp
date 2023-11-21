#include "ENpch.h"
#include "Camera.h"
#include "Engine/Debug/Assert.h"

#include <glm/gtc/matrix_transform.hpp>

namespace eng
{
  Camera::Camera()
    : m_Projection(math::Mat4(1)),
      m_ProjectionType(ProjectionType::Perspective),
      m_AspectRatio(1280.0f / 720),
      m_NearClip(-1.0f),
      m_FarClip(1.0f),
      m_FOV(80_deg),
      m_OrthographicSize(1.0f) {}

  const math::Mat4& Camera::projectionMatrix() const { return m_Projection; }
  Camera::ProjectionType Camera::projectionType() const { return m_ProjectionType; }

  f32 Camera::nearClip() const { return m_NearClip; }
  f32 Camera::farClip() const { return m_FarClip; }

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

  void Camera::setOrthographicView(f32 aspectRatio, f32 size, f32 nearClip, f32 farClip)
  {
    m_AspectRatio = aspectRatio;
    m_OrthographicSize = size;
    m_NearClip = nearClip;
    m_FarClip = farClip;
    m_ProjectionType = ProjectionType::Orthographic;

    recalculateOrthographicProjection();
  }

  void Camera::setPerspectiveView(f32 aspectRatio, math::Angle fov, f32 nearClip, f32 farClip)
  {
    m_AspectRatio = aspectRatio;
    m_FOV = fov;
    m_NearClip = nearClip;
    m_FarClip = farClip;
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
    throw std::invalid_argument("Invalid projection type!");
  }

  void Camera::recalculateOrthographicProjection()
  {
    f32 left = -m_OrthographicSize * m_AspectRatio / 2;
    f32 right = m_OrthographicSize * m_AspectRatio / 2;
    f32 bottom = -m_OrthographicSize / 2;
    f32 top = m_OrthographicSize / 2;

    m_Projection = glm::ortho(left, right, bottom, top, m_NearClip, m_FarClip);
  }

  void Camera::recalculatePerspectiveProjection()
  {
    m_Projection = glm::perspective(m_FOV.radf(), m_AspectRatio, m_NearClip, m_FarClip);
  }
}