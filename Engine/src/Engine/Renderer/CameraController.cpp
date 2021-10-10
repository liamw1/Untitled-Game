#include "ENpch.h"
#include "CameraController.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Input.h"

namespace Engine
{
  static constexpr radians s_MinPitch = glm::radians(-89.99f);
  static constexpr radians s_MaxPitch = glm::radians(89.99f);

  CameraController::CameraController(radians fov, float aspectRatio, float nearPlane, float farPlane)
    : m_Fov(fov),
      m_AspectRatio(aspectRatio),
      m_NearPlane(nearPlane),
      m_FarPlane(farPlane),
      m_Camera(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane),
      m_LastMousePosition(Application::Get().getWindow().getWidth() / 2, Application::Get().getWindow().getHeight() / 2)
  {
    setMouseState(isMouseEnabled);
  }

  void CameraController::onUpdate(std::chrono::duration<float> timestep)
  {
    const float dt = timestep.count();  // Time between frames in seconds

    if (Input::IsKeyPressed(Key::A))
    {
      m_CameraPosition.x += sinf(yaw) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.z -= cosf(yaw) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::D))
    {
      m_CameraPosition.x -= sinf(yaw) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.z += cosf(yaw) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::W))
    {
      m_CameraPosition.x += cosf(yaw) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.z += sinf(yaw) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::S))
    {
      m_CameraPosition.x -= cosf(yaw) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.z -= sinf(yaw) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::Space))
      m_CameraPosition.y += m_CameraTranslationSpeed * dt;
    if (Input::IsKeyPressed(Key::LeftShift))
      m_CameraPosition.y -= m_CameraTranslationSpeed * dt;

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
    yaw += (event.getX() - m_LastMousePosition.x) * m_CameraSensitivity;
    pitch -= (event.getY() - m_LastMousePosition.y) * m_CameraSensitivity;

    pitch = std::max(pitch, s_MinPitch);
    pitch = std::min(pitch, s_MaxPitch);

    // Convert from spherical coordinates to Cartesian coordinates
    m_CameraViewDirection.x = cos(yaw) * cos(pitch);
    m_CameraViewDirection.y = sin(pitch);
    m_CameraViewDirection.z = sin(yaw) * cos(pitch);

    m_Camera.setView(m_CameraPosition, m_CameraViewDirection);

    m_LastMousePosition.x = event.getX();
    m_LastMousePosition.y = event.getY();

    return true;
  }

  bool CameraController::onMouseScroll(MouseScrollEvent& event)
  {
    m_Fov -= m_CameraZoomSensitivity * glm::radians(event.getYOffset());
    m_Fov = std::max(m_Fov, glm::radians(5.0f));
    m_Fov = std::min(m_Fov, glm::radians(60.0f));

    m_Camera.setProjection(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
    return false;
  }

  bool CameraController::onWindowResize(WindowResizeEvent& event)
  {
    m_AspectRatio = event.getHeight() == 0 ? 0.0f : static_cast<float>(event.getWidth()) / event.getHeight();
    m_Camera.setProjection(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
    return false;
  }

  bool CameraController::onKeyPress(KeyPressEvent& event)
  {
    if (event.getKeyCode() == Key::Escape)
      isMouseEnabled = !isMouseEnabled;

    setMouseState(isMouseEnabled);
    return true;
  }

  void CameraController::setMouseState(bool enableMouse)
  {
    enableMouse ? Application::Get().getWindow().enableCursor() : Application::Get().getWindow().disableCursor();
  }
}
