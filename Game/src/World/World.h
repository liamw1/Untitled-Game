#pragma once
#include "ChunkManager.h"

struct RayIntersection
{
  length_t distance = std::numeric_limits<length_t>::max();
  eng::math::Direction face;
  BlockIndex blockIndex;
  LocalIndex chunkIndex;
};

class World
{
public:
  void initialize();

  void onUpdate(eng::Timestep timestep);

  void onEvent(eng::event::Event& event);

private:
  static constexpr length_t c_MinDistanceToWall = 0.01_m * block::length();

  bool m_RenderingPaused = false;

  ChunkManager m_ChunkManager;

  RayIntersection m_PlayerRayCast;

  /*
    \returns The first intersection between the given line segment AB and a solid block Face.
  */
  RayIntersection castRaySegment(const eng::math::Vec3& pointA, const eng::math::Vec3& pointB) const;

  RayIntersection castRay(const eng::math::Vec3& rayOrigin, const eng::math::Vec3& rayDirection, length_t maxDistance) const;

  void playerCollisionHandling(eng::Timestep timestep) const;
  void playerWorldInteraction();

  bool onKeyPress(eng::event::KeyPress& event);
};