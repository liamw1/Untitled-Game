#pragma once
#include "Camera.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/KeyEvent.h"

namespace Engine
{
  class CameraController
  {
  public:
    CameraController(float fov, float aspectRatio, float nearPlane, float farPlane);

    void onUpdate(std::chrono::duration<float> timestep);
    void onEvent(Event& event);

    inline Camera& getCamera() { return m_Camera; }
    inline const Camera& getCamera() const { return m_Camera; }

  private:
    bool isMouseEnabled = true;
    float m_Fov;
    float m_AspectRatio;
    float m_NearPlane;
    float m_FarPlane;
    float m_ZoomLevel = 1.0f;
    Camera m_Camera;

    radians m_CameraRotation = 0.0f;
    glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };

    radiansPerSec m_CameraRotationSpeed = PI;
    float m_CameraTranslationSpeed = 3.0f;

    bool onMouseScroll(MouseScrollEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
    bool onKeyPress(KeyPressEvent& event);
  };
}