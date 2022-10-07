#pragma once
#define USE_MT true

#if USE_MT
#include "MTChunkManager.h"
#else
#include "ChunkManager.h"
#endif

struct RayIntersection
{
  length_t distance = std::numeric_limits<length_t>::max();
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
  static constexpr length_t s_MinDistanceToWall = 0.001_m * Block::Length();

  bool m_RenderingPaused = false;

#if USE_MT
  MTChunkManager m_ChunkManager{};
#else
  ChunkManager m_ChunkManager{};
#endif

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