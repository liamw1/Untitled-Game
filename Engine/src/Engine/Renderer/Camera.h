#pragma once

namespace Engine
{
  class Camera
  {
  public:
    enum class ProjectionType { Perspective = 0, Orthographic = 1 };

  public:
    Camera() = default;

    const Mat4& getProjection() const { return m_Projection; }
    ProjectionType getProjectionType() const { return m_ProjectionType; }

    float getAspectRatio() const { return m_AspectRatio; }
    void changeAspectRatio(float aspectRatio);
    float getNearClip() const { return m_NearClip; }
    float getFarClip() const { return m_FarClip; }

    Angle getFOV() const;
    void changeFOV(Angle fov);

    float getOrthographicSize() const;

    void setOrthographic(float aspectRatio, float size, float nearClip, float farClip);
    void setPerspective(float aspectRatio, Angle fov, float nearClip, float farClip);

    void setViewportSize(uint32_t width, uint32_t height);

  private:
    Mat4 m_Projection = Mat4(1.0);
    ProjectionType m_ProjectionType = ProjectionType::Perspective;

    // Shared parameters
    float m_AspectRatio = 1280.0f / 720.0f;
    float m_NearClip = -1.0f;
    float m_FarClip = 1.0f;

    // Projection-specific parameters
    Angle m_FOV = Angle(80.0f);
    float m_OrthographicSize = 1.0f;

    void recalculateProjection();
    void recalculateOrthographicProjection();
    void recalculatePerspectiveProjection();
  };
}