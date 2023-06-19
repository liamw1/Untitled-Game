#pragma once
#include "ChunkManager.h"

struct RayIntersection
{
  length_t distance = std::numeric_limits<length_t>::max();
  Direction face;
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
  static constexpr length_t c_MinDistanceToWall = 0.001_m * Block::Length();

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