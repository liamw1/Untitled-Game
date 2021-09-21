#pragma once
#include <glm/glm.hpp>

namespace Engine
{
  class Camera
  {
  public:
    Camera(float fov, float aspectRatio, float nearPlane, float farPlane);

    void setProjection(float fov, float aspectRatio, float nearPlane, float farPlane);

    inline const glm::vec3& getPosition() const { return m_Position; }
    inline void setPosition(const glm::vec3& position) { m_Position = position; recalculateViewMatrix(); }

    inline radians getRotation() const { return m_Rotation; }
    inline void setRotation(radians rotation) { m_Rotation = rotation; recalculateViewMatrix(); }

    inline const glm::mat4& getPerspectiveMatrix() const { return m_PerspectiveMatrix; }
    inline const glm::mat4& getViewMatrix() const { return m_ViewMatrix; }
    inline const glm::mat4& getViewPerspectiveMatrix() const { return m_ViewPerspectiveMatrix; }

  private:
    glm::mat4 m_PerspectiveMatrix;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ViewPerspectiveMatrix;

    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    radians m_Rotation = 0.0f;

    void recalculateViewMatrix();
  };
}