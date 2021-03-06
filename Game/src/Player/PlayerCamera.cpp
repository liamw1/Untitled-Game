#include "GMpch.h"
#include "PlayerCamera.h"
#include "Player/Player.h"

CameraController::CameraController(Engine::Entity entity)
  : m_Entity(entity)
{
  Component::Camera& cameraComponent = m_Entity.get<Component::Camera>();
  cameraComponent.isActive = true;
  cameraComponent.camera.setPerspective(s_AspectRatio, s_FOV, s_NearClip, s_FarClip);
}

void CameraController::onUpdate(Timestep timestep)
{
  const seconds dt = timestep.sec();  // Time between frames in seconds
  Vec3 viewDirection = m_Entity.get<Component::Transform>().orientationDirection();
  Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));

  // Update player velocity
  Vec3 velocity{};
  if (Engine::Input::IsKeyPressed(Key::A))
    velocity += Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  if (Engine::Input::IsKeyPressed(Key::D))
    velocity -= Vec3(s_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
  if (Engine::Input::IsKeyPressed(Key::W))
    velocity += Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
  if (Engine::Input::IsKeyPressed(Key::S))
    velocity -= Vec3(s_TranslationSpeed * planarViewDirection, 0.0);
  if (Engine::Input::IsKeyPressed(Key::Space))
    velocity.z += s_TranslationSpeed;
  if (Engine::Input::IsKeyPressed(Key::LeftShift))
    velocity.z -= s_TranslationSpeed;

  // Update player position
  m_Entity.get<Component::Transform>().position += velocity * dt;

  // TODO: Remove
  Player::SetVelocity(velocity);
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
  yaw += Angle((event.getX() - m_LastMousePosition.x) * s_CameraSensitivity);
  pitch += Angle((event.getY() - m_LastMousePosition.y) * s_CameraSensitivity);

  pitch = std::max(pitch, s_MinPitch);
  pitch = std::min(pitch, s_MaxPitch);

  transformComponent.rotation = Vec3(roll.rad(), pitch.rad(), yaw.rad());

  m_LastMousePosition.x = event.getX();
  m_LastMousePosition.y = event.getY();

  return true;
}

bool CameraController::onMouseScroll(Engine::MouseScrollEvent& event)
{
  Engine::Camera& camera = m_Entity.get<Component::Camera>().camera;

  Angle cameraFOV = camera.getFOV();
  cameraFOV -= s_CameraZoomSensitivity * event.getYOffset() * cameraFOV;
  cameraFOV = std::max(cameraFOV, s_MinFOV);
  cameraFOV = std::min(cameraFOV, s_MaxFOV);

  camera.changeFOV(cameraFOV);
  return true;
}