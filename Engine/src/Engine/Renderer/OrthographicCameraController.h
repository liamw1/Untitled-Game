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

    void onUpdate(std::chrono::duration<seconds> timestep);
    void onEvent(Event& event);

    OrthographicCamera& getCamera() { return m_Camera; }
    const OrthographicCamera& getCamera() const { return m_Camera; }

  private:
    bool m_CanRotate;
    float m_AspectRatio;
    float m_ZoomLevel = 1.0f;
    OrthographicCamera m_Camera;

    radians m_CameraRotation = 0.0;
    Vec3 m_CameraPosition = { 0.0, 0.0, 0.0 };

    radiansPerSec m_CameraRotationSpeed = PI;
    length_t m_CameraTranslationSpeed = 3.0;

    bool onMouseScroll(MouseScrollEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
  };
}