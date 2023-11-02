#pragma once
#include "Engine/Math/Angle.h"
#include "Engine/Math/Vec.h"

namespace eng
{
  class Camera
  {
  public:
    enum class ProjectionType { Perspective, Orthographic };

  private:
    math::Mat4 m_Projection;
    ProjectionType m_ProjectionType;

    // Shared parameters
    f32 m_AspectRatio;
    f32 m_NearClip;
    f32 m_FarClip;

    // Projection-specific parameters
    math::Angle m_FOV;
    f32 m_OrthographicSize;

  public:
    Camera();

    const math::Mat4& projectionMatrix() const;
    ProjectionType projectionType() const;

    f32 nearClip() const;
    f32 farClip() const;

    f32 aspectRatio() const;
    void setAspectRatio(f32 aspectRatio);

    math::Angle fov() const;
    void setFov(math::Angle fov);

    f32 orthographicSize() const;

    void setOrthographicView(f32 aspectRatio, f32 size, f32 nearClip, f32 farClip);
    void setPerspectiveView(f32 aspectRatio, math::Angle fov, f32 nearClip, f32 farClip);

    void setViewportSize(u32 width, u32 height);

  private:
    void recalculateProjection();
    void recalculateOrthographicProjection();
    void recalculatePerspectiveProjection();
  };
}