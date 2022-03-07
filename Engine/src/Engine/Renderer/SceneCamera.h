#pragma once

namespace Engine
{
  class SceneCamera
  {
  public:
    SceneCamera() = default;

    const Mat4& getProjection() const { return m_Projection; }

    // TEMP 
    void setProjection(const Mat4& proj) { m_Projection = proj; }

    void setOrthographic(float aspectRatio, float size, float nearClip, float farClip);
    void setPerspective(float aspectRatio, float fov, float nearClip, float farClip);

    void setViewportSize(uint32_t width, uint32_t height);

  private:
    Mat4 m_Projection = Mat4(1.0);
    std::array<float, 4> m_CameraParameters{};
    bool m_Orthographic = false;

    void recalculateOrthographicProjection();
    void recalculatePerspectiveProjection();
  };
}