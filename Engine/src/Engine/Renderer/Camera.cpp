#include "ENpch.h"
#include "Camera.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  Camera::Camera(radians fov, float aspectRatio, float nearPlane, float farPlane)
    : m_ProjectionMatrix(glm::perspective(fov, aspectRatio, nearPlane, farPlane)),
      m_ViewMatrix(1.0f),
      m_ViewProjectionMatrix(m_ProjectionMatrix * m_ViewMatrix)
  {
  }

  void Camera::setPosition(const glm::vec3& position)
  {
    m_Position = position;
    recalculateViewMatrix();
  }

  void Camera::setView(const glm::vec3& position, const glm::vec3& viewDirection)
  {
    m_Position = position;
    m_ViewDirection = viewDirection;
    recalculateViewMatrix();
  }

  void Camera::setProjection(radians fov, float aspectRatio, float nearPlane, float farPlane)
  {
    m_ProjectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
  }

  void Camera::recalculateViewMatrix()
  {
    m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_ViewDirection, s_UpDirection);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
  }
}
