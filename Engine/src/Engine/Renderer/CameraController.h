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
    CameraController(radians fov, float aspectRatio, float nearPlane, float farPlane);

    void onUpdate(std::chrono::duration<float> timestep);
    void onEvent(Event& event);

    void setPosition(const glm::vec3& position) { m_CameraPosition = position; }

    Camera& getCamera() { return m_Camera; }
    const Camera& getCamera() const { return m_Camera; }
    const glm::vec3& getViewDirection() const { return m_CameraViewDirection; }

  private:
    // Camera state
    bool m_IsMouseEnabled = false;
    bool m_FreeCamEnabled = false;
    float m_Fov;
    float m_AspectRatio;
    float m_NearPlane;
    float m_FarPlane;
    Camera m_Camera;

    radians m_Roll = 0.0f;    // Rotation around x-axis
    radians m_Pitch = 0.0f;   // Rotation around y-axis
    radians m_Yaw = -PI / 2;  // Rotation around z-axis
    glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_CameraViewDirection = { 0.0f, 1.0f, 0.0f };

    float m_CameraTranslationSpeed = 5.0f;
    float m_CameraSensitivity = 0.0015f;
    float m_CameraZoomSensitivity = 2.0f;

    // Mouse state
    glm::vec2 m_LastMousePosition;

    bool onMouseMove(MouseMoveEvent& event);
    bool onMouseScroll(MouseScrollEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
    bool onKeyPress(KeyPressEvent& event);

    void setMouseState(bool enableMouse);
  };
}