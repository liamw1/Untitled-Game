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
      m_Camera(m_Fov, m_AspectRatio * m_ZoomLevel, m_NearPlane, m_FarPlane),
      m_WindowWidth(Application::Get().getWindow().getWidth()),
      m_WindowHeight(Application::Get().getWindow().getHeight())
  {
  }

  void CameraController::onUpdate(std::chrono::duration<float> timestep)
  {
    EN_PROFILE_FUNCTION();

    const float dt = timestep.count();  // Time between frames in seconds

    if (Input::IsKeyPressed(Key::A))
    {
      m_CameraPosition.x -= cosf(0) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y -= sinf(0) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::D))
    {
      m_CameraPosition.x += cosf(0) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y += sinf(0) * m_CameraTranslationSpeed * dt;
    }

    if (Input::IsKeyPressed(Key::W))
    {
      m_CameraPosition.x += -sinf(0) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y += cosf(0) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::S))
    {
      m_CameraPosition.x -= -sinf(0) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y -= cosf(0) * m_CameraTranslationSpeed * dt;
    }

    m_Camera.setPosition(m_CameraPosition);
  }

  void CameraController::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<MouseScrollEvent>(EN_BIND_EVENT_FN(onMouseScroll));
    dispatcher.dispatch<WindowResizeEvent>(EN_BIND_EVENT_FN(onWindowResize));
    dispatcher.dispatch<KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPress));
  }

  bool CameraController::onMouseScroll(MouseScrollEvent& event)
  {
    EN_PROFILE_FUNCTION();
    // TODO

    m_ZoomLevel -= event.getYOffset() * 0.1f * m_ZoomLevel;
    m_ZoomLevel = std::max(m_ZoomLevel, 0.1f);
    m_Camera.setProjection(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
    return false;
  }

  bool CameraController::onWindowResize(WindowResizeEvent& event)
  {
    EN_PROFILE_FUNCTION();
    // TODO

    m_WindowWidth = event.getWidth();
    m_WindowHeight = event.getHeight();
    m_AspectRatio = m_WindowHeight == 0 ? 0.0f : (float)m_WindowWidth / (float)m_WindowHeight;

    m_Camera.setProjection(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
    return false;
  }

  bool CameraController::onKeyPress(KeyPressEvent& event)
  {
    if (event.getKeyCode() == Key::Escape)
      isMouseEnabled = !isMouseEnabled;

    isMouseEnabled ? Application::Get().getWindow().enableCursor() : Application::Get().getWindow().disableCursor();
    return true;
  }
}
