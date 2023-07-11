#pragma once

namespace Engine
{
  class Camera
  {
  public:
    enum class ProjectionType { Perspective, Orthographic };

  public:
    Camera();

    const Mat4& projectionMatrix() const;
    ProjectionType projectionType() const;

    float nearClip() const;
    float farClip() const;

    float aspectRatio() const;
    void setAspectRatio(float aspectRatio);

    Angle fov() const;
    void setFov(Angle fov);

    float orthographicSize() const;

    void setOrthographicView(float aspectRatio, float size, float nearClip, float farClip);
    void setPerspectiveView(float aspectRatio, Angle fov, float nearClip, float farClip);

    void setViewportSize(uint32_t width, uint32_t height);

  private:
    Mat4 m_Projection;
    ProjectionType m_ProjectionType;

    // Shared parameters
    float m_AspectRatio;
    float m_NearClip;
    float m_FarClip;

    // Projection-specific parameters
    Angle m_FOV;
    float m_OrthographicSize;

    void recalculateProjection();
    void recalculateOrthographicProjection();
    void recalculatePerspectiveProjection();
  };
}