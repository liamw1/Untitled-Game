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

    void onUpdate(std::chrono::duration<int64_t, std::nano> timestep);
    void onEvent(Event& event);

    inline OrthographicCamera& getCamera() { return m_Camera; }
    inline const OrthographicCamera& getCamera() const { return m_Camera; }

  private:
    bool m_CanRotate;
    float m_AspectRatio;
    float m_ZoomLevel = 1.0f;
    OrthographicCamera m_Camera;

    float m_CameraRotation = 0.0f;
    glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };

    float m_CameraRotationSpeed = 180.0f;
    float m_CameraTranslationSpeed = 3.0f;

    bool onMouseScroll(MouseScrollEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
  };
}