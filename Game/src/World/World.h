#pragma once
#include "ChunkManager.h"
#include "Player/Player.h"

struct RayIntersection
{
  length_t distance = std::numeric_limits<length_t>::infinity();
  Block::Face face;
  BlockIndex blockIndex;
  LocalIndex chunkIndex;
};

class World
{
public:
  void initialize();

  void onUpdate(Timestep timestep);

  void onEvent(Engine::Event& event);

private:
  static constexpr length_t s_MinDistanceToWall = static_cast<length_t>(0.001 * Block::Length());

  bool m_RenderingPaused = false;

  ChunkManager m_ChunkManager{};

  RayIntersection m_PlayerRayCast{};

  /*
    \returns The first intersection between the given line segment AB and a solid Block Face.
  */
  RayIntersection castRaySegment(const Vec3& pointA, const Vec3& pointB) const;

  RayIntersection castRay(const Vec3& rayOrigin, const Vec3& rayDirection, length_t maxDistance) const;

  void playerCollisionHandling(Timestep timestep) const;
  void playerWorldInteraction();

  bool onKeyPressEvent(Engine::KeyPressEvent& event);
};