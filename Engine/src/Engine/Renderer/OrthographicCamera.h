#pragma once
#include <glm/glm.hpp>

/*
  A 2D camera with orthographic proejection.
*/
namespace Engine
{
  class OrthographicCamera
  {
  public:
    OrthographicCamera(float left, float right, float bottom, float top);

    void setProjection(float left, float right, float bottom, float top);

    const glm::vec3& getPosition() const { return m_Position; }
    void setPosition(const glm::vec3& position) { m_Position = position; recalculateViewMatrix(); }

    radians getRotation() const { return m_Rotation; }
    void setRotation(radians rotation) { m_Rotation = rotation; recalculateViewMatrix(); }

    const glm::mat4& getProjectionMatrix() const { return m_ProjectionMatrix; }
    const glm::mat4& getViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& getViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

  private:
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ViewProjectionMatrix;

    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    radians m_Rotation = 0.0f;

    void recalculateViewMatrix();
  };
}