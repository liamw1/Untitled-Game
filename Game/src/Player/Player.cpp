#include "GMpch.h"
#include "Player.h"
#include "Engine/Core/Input.h"

Player::Player()
  : m_Timestep(0.0f),
    m_CameraController(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f),
    m_Position(0.0f),
    m_Velocity(0.0f)
{
}

void Player::updateBegin(std::chrono::duration<float> timestep)
{
  m_Timestep = timestep;
  const float dt = m_Timestep.count();  // Time between frames in seconds

  if (!m_FreeCamEnabled)
  {
    const glm::vec3& viewDirection = m_CameraController.getViewDirection();
    glm::vec2 planarViewDirection = glm::normalize(glm::vec2(viewDirection));

    // Update player velocity
    m_Velocity = glm::vec3(0.0f);
    if (Engine::Input::IsKeyPressed(Key::A))
      m_Velocity += glm::vec3(m_TranslationSpeed * glm::vec2(-planarViewDirection.y, planarViewDirection.x), 0.0f);
    if (Engine::Input::IsKeyPressed(Key::D))
      m_Velocity -= glm::vec3(m_TranslationSpeed * glm::vec2(-planarViewDirection.y, planarViewDirection.x), 0.0f);
    if (Engine::Input::IsKeyPressed(Key::W))
      m_Velocity += glm::vec3(m_TranslationSpeed * planarViewDirection, 0.0f);
    if (Engine::Input::IsKeyPressed(Key::S))
      m_Velocity -= glm::vec3(m_TranslationSpeed * planarViewDirection, 0.0f);
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
  const glm::vec3& viewDirection = m_CameraController.getViewDirection();
  glm::vec2 planarViewDirection = glm::normalize(glm::vec2(viewDirection));

  // Update camera position
  glm::vec3 eyesPosition = glm::vec3(0.4f * s_Width * planarViewDirection + glm::vec2(m_Position), m_Position.z + 0.4f * s_Height);
  m_CameraController.setPosition(eyesPosition);

  m_CameraController.onUpdate(m_Timestep);
}

void Player::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}

AABB Player::boundingBox() const
{
  glm::vec3 boundingBoxMin = m_Position - 0.5f * glm::vec3(s_Width, s_Width, s_Height);
  glm::vec3 boundingBoxMax = m_Position + 0.5f * glm::vec3(s_Width, s_Width, s_Height);

  return { boundingBoxMin, boundingBoxMax };
}
