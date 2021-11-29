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

  std::pair<float, BlockFace> castRaySegment(const glm::vec3& pointA, const glm::vec3& pointB) const;
};