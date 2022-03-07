#pragma once
#include "OrthographicCamera.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/MouseEvent.h"

namespace Engine
{
  class OrthographicCameraController
  {
  public:
    OrthographicCameraController(float aspectRatio, bool canRotate = false);

    void onUpdate(Timestep timestep);
    void onEvent(Event& event);

    void resize(float width, float height);

    OrthographicCamera& getCamera() { return m_Camera; }
    const OrthographicCamera& getCamera() const { return m_Camera; }
    const Mat4& getViewProjectionMatrix() const { return m_Camera.getViewProjectionMatrix(); }

  private:
    bool m_CanRotate;
    float m_AspectRatio;
    float m_ZoomLevel = 1.0f;
    OrthographicCamera m_Camera;

    Angle m_CameraRotation = Angle(0.0f);
    Vec3 m_CameraPosition = { 0.0, 0.0, 0.0 };

    Angle m_CameraRotationSpeed = Angle(90.0f);
    length_t m_CameraTranslationSpeed = 3.0;

    bool onMouseScroll(MouseScrollEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
  };
}