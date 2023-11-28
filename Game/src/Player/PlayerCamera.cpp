#include "GMpch.h"
#include "PlayerCamera.h"
#include "Player/Player.h"

CameraController::CameraController(eng::Entity entity)
  : m_Entity(entity),
    m_LastMousePosition(eng::math::Vec2(0))
{
  eng::component::Camera& cameraComponent = m_Entity.get<eng::component::Camera>();
  cameraComponent.isActive = true;
  cameraComponent.camera.setPerspectiveView(c_AspectRatio, c_FOV, c_NearClip);
}

void CameraController::onUpdate(eng::Timestep timestep)
{
}

void CameraController::onEvent(eng::event::Event& event)
{
  event.dispatch(&CameraController::onMouseMove, this);
  event.dispatch(&CameraController::onMouseScroll, this);
}

bool CameraController::onMouseMove(eng::event::MouseMove& event)
{
  eng::component::Transform& transformComponent = m_Entity.get<eng::component::Transform>();

  const eng::math::Vec3& rotation = transformComponent.rotation;
  eng::math::Angle roll = eng::math::Angle::FromRad(rotation.x);
  eng::math::Angle pitch = eng::math::Angle::FromRad(rotation.y);
  eng::math::Angle yaw = eng::math::Angle::FromRad(rotation.z);

  // Adjust view angles based on mouse movement
  yaw += eng::math::Angle((event.x() - m_LastMousePosition.x) * c_CameraSensitivity);
  pitch += eng::math::Angle((event.y() - m_LastMousePosition.y) * c_CameraSensitivity);

  pitch = std::max(pitch, c_MinPitch);
  pitch = std::min(pitch, c_MaxPitch);

  transformComponent.rotation = eng::math::Vec3(roll.rad(), pitch.rad(), yaw.rad());

  m_LastMousePosition.x = event.x();
  m_LastMousePosition.y = event.y();

  return true;
}

bool CameraController::onMouseScroll(eng::event::MouseScroll& event)
{
  eng::Camera& camera = m_Entity.get<eng::component::Camera>().camera;

  eng::math::Angle cameraFOV = camera.fov();
  cameraFOV -= c_CameraZoomSensitivity * event.yOffset() * cameraFOV;
  cameraFOV = std::max(cameraFOV, c_MinFOV);
  cameraFOV = std::min(cameraFOV, c_MaxFOV);

  camera.setFov(cameraFOV);
  return true;
}