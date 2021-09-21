#include "ENpch.h"
#include "CameraController.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Input.h"

namespace Engine
{
  CameraController::CameraController(float fov, float aspectRatio, float nearPlane, float farPlane)
    : m_Fov(fov),
      m_AspectRatio(aspectRatio),
      m_NearPlane(nearPlane),
      m_FarPlane(farPlane),
      m_Camera(m_Fov, m_AspectRatio * m_ZoomLevel, m_NearPlane, m_FarPlane)
  {
  }

  void CameraController::onUpdate(std::chrono::duration<float> timestep)
  {
    EN_PROFILE_FUNCTION();

    const float dt = timestep.count();  // Time between frames in seconds

    if (Input::IsKeyPressed(Key::A))
    {
      m_CameraPosition.x -= cos(m_CameraRotation) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y -= sin(m_CameraRotation) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::D))
    {
      m_CameraPosition.x += cos(m_CameraRotation) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y += sin(m_CameraRotation) * m_CameraTranslationSpeed * dt;
    }

    if (Input::IsKeyPressed(Key::W))
    {
      m_CameraPosition.x += -sin(m_CameraRotation) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y += cos(m_CameraRotation) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::S))
    {
      m_CameraPosition.x -= -sin(m_CameraRotation) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y -= cos(m_CameraRotation) * m_CameraTranslationSpeed * dt;
    }

    if (Input::IsKeyPressed(Key::Q))
      m_CameraRotation += m_CameraRotationSpeed * dt;
    if (Input::IsKeyPressed(Key::E))
      m_CameraRotation -= m_CameraRotationSpeed * dt;

    if (m_CameraRotation > PI)
      m_CameraRotation -= 2 * PI;
    else if (m_CameraRotation <= -PI)
      m_CameraRotation += 2 * PI;

    m_Camera.setRotation(m_CameraRotation);
    m_Camera.setPosition(m_CameraPosition);

    m_CameraTranslationSpeed = m_ZoomLevel;
  }

  void CameraController::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<MouseScrollEvent>(EN_BIND_EVENT_FN(onMouseScroll));
    dispatcher.dispatch<WindowResizeEvent>(EN_BIND_EVENT_FN(onWindowResize));
  }

  bool CameraController::onMouseScroll(MouseScrollEvent& event)
  {
    EN_PROFILE_FUNCTION();
    // TODO

    m_ZoomLevel -= event.getYOffset() * 0.1f * m_ZoomLevel;
    m_ZoomLevel = std::max(m_ZoomLevel, 0.1f);
    m_Camera.setProjection(m_Fov, m_AspectRatio * m_ZoomLevel, m_NearPlane, m_FarPlane);
    return false;
  }

  bool CameraController::onWindowResize(WindowResizeEvent& event)
  {
    EN_PROFILE_FUNCTION();
    // TODO

    m_AspectRatio = (float)event.getWidth() / (float)event.getHeight();
    m_Camera.setProjection(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
    return false;
  }
  bool CameraController::onKeyPress(KeyPressEvent& event)
  {
    // TODO: Have to bug fix

    if (event.getKeyCode() == Key::V)
      isMouseEnabled = !isMouseEnabled;

    isMouseEnabled ? Application::Get().getWindow().enableCursor() : Application::Get().getWindow().disableCursor();
    return true;
  }
}
