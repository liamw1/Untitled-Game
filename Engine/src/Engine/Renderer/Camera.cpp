#include "ENpch.h"
#include "Camera.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  Camera::Camera(radians fov, float aspectRatio, length_t nearPlane, length_t farPlane)
    : m_ProjectionMatrix(glm::perspective(static_cast<length_t>(fov), static_cast<length_t>(aspectRatio), nearPlane, farPlane)),
      m_ViewMatrix(1.0),
      m_ViewProjectionMatrix(m_ProjectionMatrix * m_ViewMatrix)
  {
  }

  Camera::Camera(const Camera& camera, radians fov, float aspectRatio, length_t nearPlane, length_t farPlane)
    : m_ProjectionMatrix(glm::perspective(static_cast<length_t>(fov), static_cast<length_t>(aspectRatio), nearPlane, farPlane)),
      m_ViewMatrix(camera.m_ViewMatrix),
      m_ViewProjectionMatrix(m_ProjectionMatrix * m_ViewMatrix)
  {
  }

  void Camera::setPosition(const Vec3& position)
  {
    m_Position = position;
    recalculateViewMatrix();
  }

  void Camera::setView(const Vec3& position, const Vec3& viewDirection)
  {
    m_Position = position;
    m_ViewDirection = viewDirection;
    recalculateViewMatrix();
  }

  void Camera::setProjection(radians fov, float aspectRatio, length_t nearPlane, length_t farPlane)
  {
    m_ProjectionMatrix = glm::perspective(static_cast<length_t>(fov), static_cast<length_t>(aspectRatio), nearPlane, farPlane);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
  }

  void Camera::recalculateViewMatrix()
  {
    m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_ViewDirection, s_UpDirection);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
  }
}
