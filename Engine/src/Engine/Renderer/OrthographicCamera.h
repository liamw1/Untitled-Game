#pragma once
#include <glm/glm.hpp>

namespace Engine
{
  class OrthographicCamera
  {
  public:
    OrthographicCamera(float left, float right, float bottom, float top);

    inline const glm::vec3& getPosition() const { return m_Position; }
    inline void setPosition(const glm::vec3& position) { m_Position = position; recalculateViewMatrix(); }

    inline float getRotation() const { return m_Rotation; }
    inline void setRotation(float rotation) { m_Rotation = rotation; recalculateViewMatrix(); }

    inline const glm::mat4& getProjectionMatrix() const { return m_ProjectionMatrix; }
    inline const glm::mat4& getViewMatrix() const { return m_ViewMatrix; }
    inline const glm::mat4& getViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

  private:
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ViewProjectionMatrix;

    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    float m_Rotation = 0.0f;

    void recalculateViewMatrix();
  };
}