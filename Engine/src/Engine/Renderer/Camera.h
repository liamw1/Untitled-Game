#pragma once
#include <glm/glm.hpp>

/*
  A 3D camera with prospective projection.
*/
namespace Engine
{
  class Camera
  {
  public:
    Camera(radians fov, float aspectRatio, float nearPlane, float farPlane);

    void setPosition(const glm::vec3& position);
    void setView(const glm::vec3& position, const glm::vec3& viewDirection);
    void setProjection(radians fov, float aspectRatio, float nearPlane, float farPlane);

    const glm::vec3& getPosition() const { return m_Position; }
    const glm::mat4& getProjectionMatrix() const { return m_ProjectionMatrix; }
    const glm::mat4& getViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& getViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

  private:
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ViewProjectionMatrix;

    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_ViewDirection = { 0.0f, 1.0f, 0.0f };
    constexpr static glm::vec3 s_UpDirection = { 0.0f, 0.0f, 1.0f };

    void recalculateViewMatrix();
  };
}