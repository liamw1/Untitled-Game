#include "GMpch.h"
#include "Player.h"
#include "Engine/Core/Input.h"

static constexpr radians s_FOV = static_cast<radians>(glm::radians(80.0));
static constexpr float s_AspectRatio = 1280.0f / 720.0f;
static constexpr length_t s_NearPlaneDistance = static_cast<length_t>(0.125 * Block::Length());
static constexpr length_t s_FarPlaneDistance = static_cast<length_t>(1000 * Block::Length());

Player::Player()
  : m_Timestep(0.0f),
    m_CameraController(s_FOV, s_AspectRatio, s_NearPlaneDistance, s_FarPlaneDistance),
    m_Position(0.0),
    m_Velocity(0.0)
{
}

void Player::updateBegin(std::chrono::duration<seconds> timestep)
{
  m_Timestep = timestep;
  const seconds dt = m_Timestep.count();  // Time between frames in seconds

  if (!m_FreeCamEnabled)
  {
    const Vec3& viewDirection = m_CameraController.getViewDirection();
    Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));

    // Update player velocity
    m_Velocity = Vec3(0.0);
    if (Engine::Input::IsKeyPressed(Key::A))
      m_Velocity += Vec3(m_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
    if (Engine::Input::IsKeyPressed(Key::D))
      m_Velocity -= Vec3(m_TranslationSpeed * Vec2(-planarViewDirection.y, planarViewDirection.x), 0.0);
    if (Engine::Input::IsKeyPressed(Key::W))
      m_Velocity += Vec3(m_TranslationSpeed * planarViewDirection, 0.0);
    if (Engine::Input::IsKeyPressed(Key::S))
      m_Velocity -= Vec3(m_TranslationSpeed * planarViewDirection, 0.0);
    if (Engine::Input::IsKeyPressed(Key::Space))
      m_Velocity.z += m_TranslationSpeed;
    if (Engine::Input::IsKeyPressed(Key::LeftShift))
      m_Velocity.z -= m_TranslationSpeed;

    // Update player position
    m_Position += m_Velocity * dt;
  }
}

void Player::updateEnd()
{
  const Vec3& viewDirection = m_CameraController.getViewDirection();
  Vec2 planarViewDirection = glm::normalize(Vec2(viewDirection));

  // Update camera position
  Vec3 eyesPosition = Vec3(0.05 * s_Width * planarViewDirection + Vec2(m_Position), m_Position.z + 0.25 * s_Height);
  m_CameraController.setPosition(eyesPosition);

  m_CameraController.onUpdate(m_Timestep);
}

void Player::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}

AABB Player::boundingBox() const
{
  Vec3 boundingBoxMin = m_Position - 0.5 * Vec3(s_Width, s_Width, s_Height);
  Vec3 boundingBoxMax = m_Position + 0.5 * Vec3(s_Width, s_Width, s_Height);

  return { boundingBoxMin, boundingBoxMax };
}
