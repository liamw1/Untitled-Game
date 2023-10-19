#include "ENpch.h"
#include "Camera.h"
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

  float Camera::nearClip() const { return m_NearClip; }
  float Camera::farClip() const { return m_FarClip; }

  float Camera::aspectRatio() const { return m_AspectRatio; }
  void Camera::setAspectRatio(float aspectRatio)
  {
    m_AspectRatio = aspectRatio;
    recalculatePerspectiveProjection();
  }

  math::Angle Camera::fov() const
  {
    ENG_ASSERT(m_ProjectionType == ProjectionType::Perspective, "An orthographic camera does not have a FOV!");
    return m_FOV;
  }

  void Camera::setFov(math::Angle fov)
  {
    ENG_ASSERT(m_ProjectionType == ProjectionType::Perspective, "An orthographic camera does not have a FOV!");
    m_FOV = fov;
    recalculatePerspectiveProjection();
  }

  float Camera::orthographicSize() const
  {
    ENG_ASSERT(m_ProjectionType == ProjectionType::Orthographic, "A perspective camera does not have an orthographic size!");
    return m_OrthographicSize;
  }

  void Camera::setOrthographicView(float aspectRatio, float size, float nearClip, float farClip)
  {
    m_AspectRatio = aspectRatio;
    m_OrthographicSize = size;
    m_NearClip = nearClip;
    m_FarClip = farClip;
    m_ProjectionType = ProjectionType::Orthographic;

    recalculateOrthographicProjection();
  }

  void Camera::setPerspectiveView(float aspectRatio, math::Angle fov, float nearClip, float farClip)
  {
    m_AspectRatio = aspectRatio;
    m_FOV = fov;
    m_NearClip = nearClip;
    m_FarClip = farClip;
    m_ProjectionType = ProjectionType::Perspective;

    recalculatePerspectiveProjection();
  }

  void Camera::setViewportSize(uint32_t width, uint32_t height)
  {
    m_AspectRatio = height > 0 ? static_cast<float>(width) / height : 0.0f;
    recalculateProjection();
  }

  void Camera::recalculateProjection()
  {
    switch (m_ProjectionType)
    {
      case ProjectionType::Orthographic: recalculateOrthographicProjection(); break;
      case ProjectionType::Perspective:  recalculatePerspectiveProjection();  break;
      default: ENG_CORE_ERROR("Unknown projection type!");
    }
  }

  void Camera::recalculateOrthographicProjection()
  {
    float left = -m_OrthographicSize * m_AspectRatio / 2;
    float right = m_OrthographicSize * m_AspectRatio / 2;
    float bottom = -m_OrthographicSize / 2;
    float top = m_OrthographicSize / 2;

    m_Projection = glm::ortho(left, right, bottom, top, m_NearClip, m_FarClip);
  }

  void Camera::recalculatePerspectiveProjection()
  {
    m_Projection = glm::perspective(m_FOV.radf(), m_AspectRatio, m_NearClip, m_FarClip);
  }
}