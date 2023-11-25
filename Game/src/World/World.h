#pragma once
#include "Chunk/ChunkManager.h"

struct RayIntersection
{
  length_t distance = std::numeric_limits<length_t>::max();
  eng::math::Direction face;
  BlockIndex blockIndex;
  LocalIndex chunkIndex;
};

class World
{
  ChunkManager m_ChunkManager;
  RayIntersection m_PlayerRayCast;
  bool m_RenderingPaused = false;

public:
  void initialize();

  void onUpdate(eng::Timestep timestep);

  void onEvent(eng::event::Event& event);

private:
  /*
    \returns The first intersection between the given line segment AB and a solid block Face.
  */
  RayIntersection castRaySegment(const eng::math::Vec3& pointA, const eng::math::Vec3& pointB) const;

  RayIntersection castRay(const eng::math::Vec3& rayOrigin, const eng::math::Vec3& rayDirection, length_t maxDistance) const;

  void playerCollisionHandling(eng::Timestep timestep) const;
  void playerWorldInteraction();

  bool onKeyPress(eng::event::KeyPress& event);
};