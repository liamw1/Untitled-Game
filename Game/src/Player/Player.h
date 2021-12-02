#pragma once
#include "Engine/Renderer/CameraController.h"
#include "Block/Block.h"

struct AABB
{
  glm::vec3 min;
  glm::vec3 max;
};

/*
  Represents the player.

  IMPORTANT: Unlike other systems, the player's update function is split into
  two stages.  The first handles player input and should be called before any
  external changes to the player's state, such as collision handling.  The second
  handles camera positioning and shoud be called after such external changes
  have been made.
*/
class Player
{
public:
  Player();

  /*
    The first stage of the player's update.  
    Adjusts player state based on player input.
  */
  void updateBegin(std::chrono::duration<float> timestep);

  /*
    The last stage of the player's update.  
    Handles final camera positioning for the frame.
  */
  void updateEnd();

  void onEvent(Engine::Event& event);

  /*
    \returns The player's axis-aligned bounding box (AABB).
  */
  AABB boundingBox() const;

  const glm::vec3& getViewDirection() const { return m_CameraController.getViewDirection(); }
  const Engine::Camera& getCamera() const { return m_CameraController.getCamera(); }

  const glm::vec3& getPosition() const { return m_Position; }
  void setPosition(const glm::vec3& position) { m_Position = position; }

  const glm::vec3& getVelocity() const { return m_Velocity; }
  void setVelocity(const glm::vec3& velocity) { m_Velocity = velocity; }

  static constexpr float Width() { return s_Width; }
  static constexpr float Height() { return s_Height; }

private:
  // Time between current frame and previous frame
  std::chrono::duration<float> m_Timestep;

  // Hitbox dimensions
  static constexpr float s_Width = 1 * Block::Length();
  static constexpr float s_Height = 2 * Block::Length();

  // Controller for player camera, which is placed at the eyes
  Engine::CameraController m_CameraController;
  bool m_FreeCamEnabled = false;

  // Position of center of the player hitbox
  glm::vec3 m_Position;
  glm::vec3 m_Velocity;

  float m_TranslationSpeed = 64 * Block::Length();
};