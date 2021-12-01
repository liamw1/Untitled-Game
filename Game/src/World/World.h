#pragma once
#include "ChunkManager.h"
#include "Player/Player.h"

struct Intersection
{
  bool intersectionOccured;
  float distance;
  BlockFace face;
  LocalIndex blockIndex;
  ChunkIndex chunkIndex;
};

class World
{
public:
  World(const glm::vec3& initialPosition);

  void onUpdate(std::chrono::duration<float> timestep, Player& player);

  void onEvent(Engine::Event& event);

private:
  static constexpr float s_MinDistanceToWall = 0.01f * Block::Length();

  bool m_RenderingPaused = false;
  ChunkManager m_ChunkManager{};

  bool onKeyPressEvent(Engine::KeyPressEvent& event);

  /*
    \returns The first intersection between the given line segment AB and a solid Block Face.
  */
  Intersection castRaySegment(const glm::vec3& pointA, const glm::vec3& pointB) const;

  void playerCollisionHandling(std::chrono::duration<float> timestep, Player& player) const;
};