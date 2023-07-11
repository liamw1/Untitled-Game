#include "GMpch.h"
#include "PlayerCamera.h"
#include "Player/Player.h"

CameraController::CameraController(Engine::Entity entity)
  : m_Entity(entity),
    m_LastMousePosition(Vec2(0))
{
  Component::Camera& cameraComponent = m_Entity.get<Component::Camera>();
  cameraComponent.isActive = true;
  cameraComponent.camera.setPerspectiveView(c_AspectRatio, c_FOV, c_NearClip, c_FarClip);
}

void CameraController::onUpdate(Engine::Timestep timestep)
{
}

void CameraController::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::MouseMoveEvent>(EN_BIND_EVENT_FN(onMouseMove));
  dispatcher.dispatch<Engine::MouseScrollEvent>(EN_BIND_EVENT_FN(onMouseScroll));
}

bool CameraController::onMouseMove(Engine::MouseMoveEvent& event)
{
  Component::Transform& transformComponent = m_Entity.get<Component::Transform>();

  const Vec3& rotation = transformComponent.rotation;
  Angle roll = Angle::FromRad(rotation.x);
  Angle pitch = Angle::FromRad(rotation.y);
  Angle yaw = Angle::FromRad(rotation.z);

  // Adjust view angles based on mouse movement
  yaw += Angle((event.x() - m_LastMousePosition.x) * c_CameraSensitivity);
  pitch += Angle((event.y() - m_LastMousePosition.y) * c_CameraSensitivity);

  pitch = std::max(pitch, c_MinPitch);
  pitch = std::min(pitch, c_MaxPitch);

  transformComponent.rotation = Vec3(roll.rad(), pitch.rad(), yaw.rad());

  m_LastMousePosition.x = event.x();
  m_LastMousePosition.y = event.y();

  return true;
}

bool CameraController::onMouseScroll(Engine::MouseScrollEvent& event)
{
  Engine::Camera& camera = m_Entity.get<Component::Camera>().camera;

  Angle cameraFOV = camera.fov();
  cameraFOV -= c_CameraZoomSensitivity * event.yOffset() * cameraFOV;
  cameraFOV = std::max(cameraFOV, c_MinFOV);
  cameraFOV = std::min(cameraFOV, c_MaxFOV);

  camera.setFov(cameraFOV);
  return true;
}