#include "ENpch.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  enum class OrthoParams { Aspect, Size, NearClip, FarClip };
  enum class PerspectParams { Aspect, FOV, NearClip, FarClip };

  void Camera::setOrthographic(float aspectRatio, float size, float nearClip, float farClip)
  {
    m_CameraParameters[static_cast<int>(OrthoParams::Aspect)] = aspectRatio;
    m_CameraParameters[static_cast<int>(OrthoParams::Size)] = size;
    m_CameraParameters[static_cast<int>(OrthoParams::NearClip)] = nearClip;
    m_CameraParameters[static_cast<int>(OrthoParams::FarClip)] = farClip;
    m_Orthographic = true;

    recalculateOrthographicProjection();
  }

  void Camera::setPerspective(float aspectRatio, Angle fov, float nearClip, float farClip)
  {
    m_CameraParameters[static_cast<int>(PerspectParams::Aspect)] = aspectRatio;
    m_CameraParameters[static_cast<int>(PerspectParams::FOV)] = fov.rad();
    m_CameraParameters[static_cast<int>(PerspectParams::NearClip)] = nearClip;
    m_CameraParameters[static_cast<int>(PerspectParams::FarClip)] = farClip;
    m_Orthographic = false;

    recalculatePerspectiveProjection();
  }

  void Camera::setViewportSize(uint32_t width, uint32_t height)
  {
    m_CameraParameters[static_cast<int>(OrthoParams::Aspect)] = height > 0 ? static_cast<float>(width) / height : 0.0f;
    m_Orthographic ? recalculateOrthographicProjection() : recalculatePerspectiveProjection();
  }

  void Camera::recalculateOrthographicProjection()
  {
    const float& aspectRatio = m_CameraParameters[static_cast<int>(OrthoParams::Aspect)];
    const float& orthographicSize = m_CameraParameters[static_cast<int>(OrthoParams::Size)];
    const float& nearClip = m_CameraParameters[static_cast<int>(OrthoParams::NearClip)];
    const float& farClip = m_CameraParameters[static_cast<int>(OrthoParams::FarClip)];

    float left = -orthographicSize * aspectRatio / 2;
    float right = orthographicSize * aspectRatio / 2;
    float bottom = -orthographicSize / 2;
    float top = orthographicSize / 2;

    m_Projection = glm::ortho(left, right, bottom, top, nearClip, farClip);
  }

  void Camera::recalculatePerspectiveProjection()
  {
    const float& aspectRatio = m_CameraParameters[static_cast<int>(PerspectParams::Aspect)];
    const float& FOV = m_CameraParameters[static_cast<int>(PerspectParams::FOV)];
    const float& nearClip = m_CameraParameters[static_cast<int>(PerspectParams::NearClip)];
    const float& farClip = m_CameraParameters[static_cast<int>(PerspectParams::FarClip)];

    m_Projection = glm::perspective(FOV, aspectRatio, nearClip, farClip);
  }
}