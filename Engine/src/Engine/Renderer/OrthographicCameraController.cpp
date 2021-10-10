#include "ENpch.h"
#include "OrthographicCameraController.h"
#include "Engine/Core/Input.h"

namespace Engine
{
  OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool canRotate)
    : m_CanRotate(canRotate),
      m_AspectRatio(aspectRatio),
      m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio* m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel)
  {
  }

  void OrthographicCameraController::onUpdate(std::chrono::duration<float> timestep)
  {
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
      m_CameraPosition.y +=  cos(m_CameraRotation) * m_CameraTranslationSpeed * dt;
    }
    if (Input::IsKeyPressed(Key::S))
    {
      m_CameraPosition.x -= -sin(m_CameraRotation) * m_CameraTranslationSpeed * dt;
      m_CameraPosition.y -=  cos(m_CameraRotation) * m_CameraTranslationSpeed * dt;
    }

    if (m_CanRotate)
    {
      if (Input::IsKeyPressed(Key::Q))
        m_CameraRotation += m_CameraRotationSpeed * dt;
      if (Input::IsKeyPressed(Key::E))
        m_CameraRotation -= m_CameraRotationSpeed * dt;

      if (m_CameraRotation > PI)
        m_CameraRotation -= 2 * PI;
      else if (m_CameraRotation <= -PI)
        m_CameraRotation += 2 * PI;

      m_Camera.setRotation(m_CameraRotation);
    }

    m_Camera.setPosition(m_CameraPosition);

    m_CameraTranslationSpeed = m_ZoomLevel;
  }

  void OrthographicCameraController::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<MouseScrollEvent>(EN_BIND_EVENT_FN(onMouseScroll));
    dispatcher.dispatch<WindowResizeEvent>(EN_BIND_EVENT_FN(onWindowResize));
  }

  bool OrthographicCameraController::onMouseScroll(MouseScrollEvent& event)
  {
    m_ZoomLevel -= event.getYOffset() * 0.1f * m_ZoomLevel;
    m_ZoomLevel = std::max(m_ZoomLevel, 0.1f);
    m_Camera.setProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    return false;
  }

  bool OrthographicCameraController::onWindowResize(WindowResizeEvent& event)
  {
    m_AspectRatio = event.getHeight() == 0.0f ? 0.0f : static_cast<float>(event.getWidth()) / event.getHeight();
    m_Camera.setProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    return false;
  }
}