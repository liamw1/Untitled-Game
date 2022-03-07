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
    CameraController(Angle FOV, float aspectRatio, length_t nearPlane, length_t farPlane);

    void onUpdate(Timestep timestep);
    void onEvent(Event& event);

    void setPosition(const Vec3& position) { m_CameraPosition = position; }

    Camera& getCamera() { return m_Camera; }
    const Camera& getCamera() const { return m_Camera; }
    const Vec3& getViewDirection() const { return m_CameraViewDirection; }
    const Mat4& getViewProjectionMatrix() const { return m_Camera.getViewProjectionMatrix(); }

    Angle getFOV() const { return m_FOV; }
    float getAspectRatio() const { return m_AspectRatio; }

    void toggleFreeCam() { m_FreeCamEnabled = !m_FreeCamEnabled; }

  private:
    // Camera state
    bool m_IsMouseEnabled = false;
    bool m_FreeCamEnabled = false;
    Angle m_FOV;
    float m_AspectRatio;
    length_t m_NearPlane;
    length_t m_FarPlane;
    Camera m_Camera;

    Angle m_Roll = Angle(0.0f);     // Rotation around x-axis
    Angle m_Pitch = Angle(0.0f);    // Rotation around y-axis
    Angle m_Yaw = Angle(-90.0f);    // Rotation around z-axis
    Vec3 m_CameraPosition = { 0.0, 0.0, 0.0 };
    Vec3 m_CameraViewDirection = { 0.0, 1.0, 0.0 };

    length_t m_CameraTranslationSpeed = 5.0;
    float m_CameraSensitivity = 0.1f;
    float m_CameraZoomSensitivity = 0.2f;

    // Mouse state
    Float2 m_LastMousePosition;

    bool onMouseMove(MouseMoveEvent& event);
    bool onMouseScroll(MouseScrollEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
    bool onKeyPress(KeyPressEvent& event);

    void setMouseState(bool enableMouse);
  };
}