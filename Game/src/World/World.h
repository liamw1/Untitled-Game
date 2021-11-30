#pragma once
#include "ChunkManager.h"
#include "Player/Player.h"

class World
{
public:
  World(const glm::vec3& initialPosition);

  void onUpdate(std::chrono::duration<float> timestep, Player& player);

  void onEvent(Engine::Event& event);

private:
  bool m_RenderingPaused = false;
  ChunkManager m_ChunkManager{};

  bool onKeyPressEvent(Engine::KeyPressEvent& event);

  /*
    \returns The first intersection between the given line segment AB and a solid Block Face.
             The float represents how far along the segment the intersectioned occured, i.e.,
             0.5f means it occured halfway between A and B.  Will never return a negative value,
             but may return a value > 1.0f, which indicates that no intersection was found.
  */
  std::pair<float, BlockFace> castRaySegment(const glm::vec3& pointA, const glm::vec3& pointB) const;
};