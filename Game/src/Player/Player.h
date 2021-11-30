#pragma once
#include "Engine/Renderer/CameraController.h"
#include "Block/Block.h"

struct AABB
{
  glm::vec3 min;
  glm::vec3 max;
};

class Player
{
public:
  Player();

  void updateBegin(std::chrono::duration<float> timestep);
  void updateEnd();
  void onEvent(Engine::Event& event);

  AABB boundingBox() const;

  Engine::Camera& getCamera() { return m_CameraController.getCamera(); }
  const Engine::Camera& getCamera() const { return m_CameraController.getCamera(); }

  const glm::vec3& getPosition() const { return m_Position; }
  void setPosition(const glm::vec3& position) { m_Position = position; }

  const glm::vec3& getVelocity() const { return m_Velocity; }
  void setVelocity(const glm::vec3& velocity) { m_Velocity = velocity; }

  static constexpr float Width() { return s_Width; }
  static constexpr float Height() { return s_Height; }

private:
  std::chrono::duration<float> m_Timestep;

  // Hitbox dimensions
  static constexpr float s_Width = 1 * Block::Length();
  static constexpr float s_Height = 2 * Block::Length();

  // Controller for player camera, which is placed at the eyes
  Engine::CameraController m_CameraController;
  bool m_FreeCamEnabled = false;

  // Position of center of the player hitbosx
  glm::vec3 m_Position;
  glm::vec3 m_Velocity;

  float m_TranslationSpeed = 16 * Block::Length();
};