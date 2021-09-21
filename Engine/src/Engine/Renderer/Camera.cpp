#include "ENpch.h"
#include "Camera.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
    : m_PerspectiveMatrix(glm::perspective(fov, aspectRatio, nearPlane, farPlane)),
      m_ViewMatrix(1.0f),
      m_ViewPerspectiveMatrix(m_PerspectiveMatrix * m_ViewMatrix)
  {
  }

  void Camera::setProjection(float fov, float aspectRatio, float nearPlane, float farPlane)
  {
    m_PerspectiveMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
    m_ViewPerspectiveMatrix = m_PerspectiveMatrix * m_ViewMatrix;
  }

  void Camera::recalculateViewMatrix()
  {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) *
      glm::rotate(glm::mat4(1.0f), m_Rotation, glm::vec3(0, 0, 1));

    m_ViewMatrix = glm::inverse(transform);
    m_ViewPerspectiveMatrix = m_PerspectiveMatrix * m_ViewMatrix;
  }
}
