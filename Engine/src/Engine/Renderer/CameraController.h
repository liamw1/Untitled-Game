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
    CameraController();
    CameraController(radians fov, float aspectRatio, length_t nearPlane, length_t farPlane);

    void onUpdate(std::chrono::duration<seconds> timestep);
    void onEvent(Event& event);

    void setPosition(const Vec3& position) { m_CameraPosition = position; }

    Camera& getCamera() { return m_Camera; }
    const Camera& getCamera() const { return m_Camera; }
    const Vec3& getViewDirection() const { return m_CameraViewDirection; }

  private:
    // Camera state
    bool m_IsMouseEnabled = false;
    bool m_FreeCamEnabled = false;
    radians m_Fov;
    float m_AspectRatio;
    length_t m_NearPlane;
    length_t m_FarPlane;
    Camera m_Camera;

    radians m_Roll = 0.0;    // Rotation around x-axis
    radians m_Pitch = 0.0;   // Rotation around y-axis
    radians m_Yaw = -PI / 2;  // Rotation around z-axis
    Vec3 m_CameraPosition = { 0.0, 0.0, 0.0 };
    Vec3 m_CameraViewDirection = { 0.0, 1.0, 0.0 };

    length_t m_CameraTranslationSpeed = 5.0;
    float m_CameraSensitivity = 0.0015f;
    float m_CameraZoomSensitivity = 5.0f;

    // Mouse state
    Float2 m_LastMousePosition;

    bool onMouseMove(MouseMoveEvent& event);
    bool onMouseScroll(MouseScrollEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
    bool onKeyPress(KeyPressEvent& event);

    void setMouseState(bool enableMouse);
  };
}