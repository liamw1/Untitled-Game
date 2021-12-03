#pragma once
#include "Engine/Renderer/CameraController.h"
#include "World/Chunk.h"

struct AABB
{
  Vec3 min;
  Vec3 max;
};

/*
  Represents the player.

  IMPORTANT: Unlike other systems, the player's update function is split into
  two stages.  The first handles player input and should be called before any
  external changes to the player's state, such as collision handling.  The second
  handles camera positioning and shoud be called after such external changes
  have been made.

  Note: Should probably make this a namespace but too lazy to do it now.
*/
class Player
{
public:
  Player(const GlobalIndex& initialChunkIndex, const Vec3& initialLocalPosition);

  /*
    The first stage of the player's update.  
    Adjusts player state based on player input.
  */
  void updateBegin(std::chrono::duration<seconds> timestep);

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

  const Vec3& getViewDirection() const { return m_CameraController.getViewDirection(); }
  const Engine::Camera& getCamera() const { return m_CameraController.getCamera(); }

  const Vec3& getPosition() const { return m_LocalPosition; }
  void setPosition(const Vec3& position) { m_LocalPosition = position; }

  const Vec3& getVelocity() const { return m_Velocity; }
  void setVelocity(const Vec3& velocity) { m_Velocity = velocity; }

  static constexpr length_t Width() { return s_Width; }
  static constexpr length_t Height() { return s_Height; }

private:
  // Time between current frame and previous frame
  std::chrono::duration<seconds> m_Timestep;

  // Hitbox dimensions
  static constexpr length_t s_Width = 1 * Block::Length();
  static constexpr length_t s_Height = 2 * Block::Length();

  // Controller for player camera, which is placed at the eyes
  Engine::CameraController m_CameraController;
  bool m_FreeCamEnabled = false;

  GlobalIndex m_OriginIndex;
  // Position of center of the player hitbox
  Vec3 m_LocalPosition;
  Vec3 m_Velocity;

  length_t m_TranslationSpeed = 32 * Block::Length();
};