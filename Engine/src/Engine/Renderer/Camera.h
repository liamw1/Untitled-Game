#pragma once

/*
  A 3D camera with prospective projection.
*/
namespace Engine
{
  class Camera
  {
  public:
    Camera(Angle fov, float aspectRatio, length_t nearPlane, length_t farPlane);
    Camera(const Camera& camera, Angle fov, float aspectRatio, length_t nearPlane, length_t farPlane);

    void setPosition(const Vec3& position);
    void setView(const Vec3& position, const Vec3& viewDirection);
    void setProjection(Angle fov, float aspectRatio, length_t nearPlane, length_t farPlane);

    const Vec3& getPosition() const { return m_Position; }
    const Mat4& getProjectionMatrix() const { return m_ProjectionMatrix; }
    const Mat4& getViewMatrix() const { return m_ViewMatrix; }
    const Mat4& getViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

  private:
    Mat4 m_ProjectionMatrix;
    Mat4 m_ViewMatrix;
    Mat4 m_ViewProjectionMatrix;

    Vec3 m_Position = { 0.0, 0.0, 0.0 };
    Vec3 m_ViewDirection = { 0.0, 1.0, 0.0 };
    constexpr static Vec3 s_UpDirection = { 0.0, 0.0, 1.0 };

    void recalculateViewMatrix();
  };
}