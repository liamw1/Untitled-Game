#include "ENpch.h"
#include "CameraController.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Input.h"

namespace Engine
{
  static constexpr Angle s_MinPitch = Angle(-89.99f);
  static constexpr Angle s_MaxPitch = Angle(89.99f);
  static constexpr Angle s_MinFOV = Angle(0.5f);
  static constexpr Angle s_MaxFOV = Angle(80.0f);

  CameraController::CameraController()
    : m_FOV(0.0),
      m_AspectRatio(0.0),
      m_NearPlane(0.0),
      m_FarPlane(0.0),
      m_Camera(m_FOV, m_AspectRatio, m_NearPlane, m_FarPlane),
      m_LastMousePosition(0.0, 0.0)
  {
  }

  CameraController::CameraController(Angle fov, float aspectRatio, length_t nearPlane, length_t farPlane)
    : m_FOV(fov),
      m_AspectRatio(aspectRatio),
      m_NearPlane(nearPlane),
      m_FarPlane(farPlane),
      m_Camera(m_FOV, m_AspectRatio, m_NearPlane, m_FarPlane),
      m_LastMousePosition(Application::Get().getWindow().getWidth() / 2, Application::Get().getWindow().getHeight() / 2)
  {
    setMouseState(m_IsMouseEnabled);
  }

  void CameraController::onUpdate(Timestep timestep)
  {
    if (m_FreeCamEnabled)
    {
      const seconds dt = timestep.sec();  // Time between frames in seconds

      if (Input::IsKeyPressed(Key::A))
      {
        m_CameraPosition.x += sin(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
        m_CameraPosition.y += cos(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
      }
      if (Input::IsKeyPressed(Key::D))
      {
        m_CameraPosition.x -= sin(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
        m_CameraPosition.y -= cos(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
      }
      if (Input::IsKeyPressed(Key::W))
      {
        m_CameraPosition.x += cos(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
        m_CameraPosition.y -= sin(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
      }
      if (Input::IsKeyPressed(Key::S))
      {
        m_CameraPosition.x -= cos(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
        m_CameraPosition.y += sin(m_Yaw.rad()) * m_CameraTranslationSpeed * dt;
      }
      if (Input::IsKeyPressed(Key::Space))
        m_CameraPosition.z += m_CameraTranslationSpeed * dt;
      if (Input::IsKeyPressed(Key::LeftShift))
        m_CameraPosition.z -= m_CameraTranslationSpeed * dt;
    }

    m_Camera.setPosition(m_CameraPosition);
  }

  void CameraController::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<MouseMoveEvent>(EN_BIND_EVENT_FN(onMouseMove));
    dispatcher.dispatch<MouseScrollEvent>(EN_BIND_EVENT_FN(onMouseScroll));
    dispatcher.dispatch<WindowResizeEvent>(EN_BIND_EVENT_FN(onWindowResize));
    dispatcher.dispatch<KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPress));
  }

  bool CameraController::onMouseMove(MouseMoveEvent& event)
  {
    // Adjust view angles based on mouse movement
    m_Yaw += Angle((event.getX() - m_LastMousePosition.x) * m_CameraSensitivity);
    m_Pitch += Angle((event.getY() - m_LastMousePosition.y) * m_CameraSensitivity);

    m_Pitch = std::max(m_Pitch, s_MinPitch);
    m_Pitch = std::min(m_Pitch, s_MaxPitch);

    // Convert from spherical coordinates to Cartesian coordinates
    m_CameraViewDirection.x = cos(m_Yaw.rad()) * cos(m_Pitch.rad());
    m_CameraViewDirection.y = -sin(m_Yaw.rad()) * cos(m_Pitch.rad());
    m_CameraViewDirection.z = -sin(m_Pitch.rad());

    m_Camera.setView(m_CameraPosition, m_CameraViewDirection);

    m_LastMousePosition.x = event.getX();
    m_LastMousePosition.y = event.getY();

    return true;
  }

  bool CameraController::onMouseScroll(MouseScrollEvent& event)
  {
    m_FOV -= m_FOV * m_CameraZoomSensitivity * event.getYOffset();
    m_FOV = std::max(m_FOV, s_MinFOV);
    m_FOV = std::min(m_FOV, s_MaxFOV);

    m_Camera.setProjection(m_FOV, m_AspectRatio, m_NearPlane, m_FarPlane);
    return false;
  }

  bool CameraController::onWindowResize(WindowResizeEvent& event)
  {
    m_AspectRatio = event.getHeight() == 0 ? 0.0f : static_cast<float>(event.getWidth()) / event.getHeight();
    m_Camera.setProjection(m_FOV, m_AspectRatio, m_NearPlane, m_FarPlane);
    return false;
  }

  bool CameraController::onKeyPress(KeyPressEvent& event)
  {
    if (event.getKeyCode() == Key::Escape)
      m_IsMouseEnabled = !m_IsMouseEnabled;

    setMouseState(m_IsMouseEnabled);
    return true;
  }

  void CameraController::setMouseState(bool enableMouse)
  {
    enableMouse ? Application::Get().getWindow().enableCursor() : Application::Get().getWindow().disableCursor();
  }
}
