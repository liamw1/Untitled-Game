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

    const Vec3& getPosition() const { return m_Position; }
    void setPosition(const Vec3& position) { m_Position = position; recalculateViewMatrix(); }

    radians getRotation() const { return m_Rotation; }
    void setRotation(radians rotation) { m_Rotation = rotation; recalculateViewMatrix(); }

    const Mat4& getProjectionMatrix() const { return m_ProjectionMatrix; }
    const Mat4& getViewMatrix() const { return m_ViewMatrix; }
    const Mat4& getViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

  private:
    Mat4 m_ProjectionMatrix;
    Mat4 m_ViewMatrix;
    Mat4 m_ViewProjectionMatrix;

    Vec3 m_Position = { 0.0, 0.0, 0.0 };
    radians m_Rotation = 0.0;

    void recalculateViewMatrix();
  };
}