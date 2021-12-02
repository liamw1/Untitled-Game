#pragma once
#include "ChunkManager.h"
#include "Player/Player.h"

struct Intersection
{
  bool intersectionOccured;
  length_t distance;
  BlockFace face;
  LocalIndex blockIndex;
  ChunkIndex chunkIndex;
};

class World
{
public:
  World(const Vec3& initialPosition);

  void onUpdate(std::chrono::duration<seconds> timestep, Player& player);

  void onEvent(Engine::Event& event);

private:
  static constexpr length_t s_MinDistanceToWall = static_cast<length_t>(0.05 * Block::Length());

  bool m_RenderingPaused = false;

  ChunkManager m_ChunkManager{};

  Intersection m_PlayerRayCast{};

  /*
    \returns The first intersection between the given line segment AB and a solid Block Face.
  */
  Intersection castRaySegment(const Vec3& pointA, const Vec3& pointB) const;

  Intersection castRay(const Vec3& rayOrigin, const Vec3& rayDirection, length_t maxDistance) const;

  void playerCollisionHandling(std::chrono::duration<seconds> timestep, Player& player) const;

  bool onMouseButtonPressEvent(Engine::MouseButtonPressEvent& event);
  bool onKeyPressEvent(Engine::KeyPressEvent& event);
};