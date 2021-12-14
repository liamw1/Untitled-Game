#pragma once
#include "Engine/Renderer/CameraController.h"
#include "World/Chunk.h"

struct AABB
{
  Vec3 min;
  Vec3 max;
};

/*
  Represents the player.  The player's position (all the positions of all other
  geometry) is stored relative to an "origin chunk", which is the chunk the player
  is currently inside.

  IMPORTANT: Unlike other systems, the player's update function is split into
  two stages.  The first handles player input and should be called before any
  external changes to the player's state, such as collision handling.  The second
  handles camera positioning and shoud be called after such external changes
  have been made.
*/
namespace Player
{
  void Initialize(const GlobalIndex& initialChunkIndex, const Vec3& initialLocalPosition);

  /*
    The first stage of the player's update.
    Adjusts player state based on player input.
  */
  void UpdateBegin(std::chrono::duration<seconds> timestep);

  /*
    The last stage of the player's update.
    Handles final camera positioning for the frame.
  */
  void UpdateEnd();

  void OnEvent(Engine::Event& event);

  /*
    \returns The player's axis-aligned bounding box (AABB).
  */
  AABB BoundingBox();

  const Vec3& ViewDirection();
  const Engine::Camera& Camera();
  const Engine::CameraController& CameraController();

  const Vec3& Position();
  void SetPosition(const Vec3& position);

  const Vec3& Velocity();
  void SetVelocity(const Vec3& velocity);

  const GlobalIndex& OriginIndex();

  length_t Width();
  length_t Height();
};