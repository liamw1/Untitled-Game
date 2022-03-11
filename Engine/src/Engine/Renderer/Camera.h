#pragma once

namespace Engine
{
  class Camera
  {
  public:
    Camera() = default;

    const Mat4& getProjection() const { return m_Projection; }

    Angle getFOV() const;
    void changeFOV(Angle fov);
    void changeAspectRatio(float aspectRatio);

    void setOrthographic(float aspectRatio, float size, float nearClip, float farClip);
    void setPerspective(float aspectRatio, Angle fov, float nearClip, float farClip);

    void setViewportSize(uint32_t width, uint32_t height);

  private:
    Mat4 m_Projection = Mat4(1.0);
    std::array<float, 4> m_CameraParameters{};
    bool m_Orthographic = false;

    void recalculateOrthographicProjection();
    void recalculatePerspectiveProjection();
  };
}