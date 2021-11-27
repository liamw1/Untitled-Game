#include "GMpch.h"
#include "World.h"
#include "ChunkRenderer.h"

World::World(const glm::vec3& initialPosition)
{
  m_ChunkManager.updatePlayerChunk(initialPosition);

  while (m_ChunkManager.loadNewChunks(10000))
  {
  }
}

void World::onUpdate(std::chrono::duration<float> timestep, Player& player)
{
  EN_PROFILE_FUNCTION();

  const float dt = timestep.count();  // Time between frames in seconds
  const Engine::Camera& playerCamera = player.getCamera();

  player.updateBegin(timestep);

  /* Player collision detection */
  glm::vec3 playerPosition = player.getPosition();
  ChunkIndex playerChunkIndex = Chunk::ChunkIndexFromPos(playerPosition);
  Chunk* playerChunk = m_ChunkManager.findChunk(playerChunkIndex);

  static const int playerWidth = static_cast<int>(ceil(Player::Width() / Block::Length()));
  static const int playerHeight = static_cast<int>(ceil(Player::Height() / Block::Length()));
  glm::vec3 anchorPoint = playerPosition - 0.5f * glm::vec3(Player::Width(), Player::Width(), Player::Height());
  for (int i = 0; i <= playerWidth; ++i)
    for (int j = 0; j <= playerWidth; ++j)
      for (int k = 0; k <= playerHeight; ++k)
      {
        glm::vec3 cornerPos = anchorPoint + Block::Length() * glm::vec3(i, j, k);

        if (Chunk::ChunkIndexFromPos(cornerPos) == playerChunkIndex && playerChunk != nullptr)
        {
          BlockType block = playerChunk->getBlockType(cornerPos);
          if (Block::HasCollision(block))
          {
            player.setPosition(playerPosition - player.getVelocity() * dt);
            EN_INFO("Collision detected!");
          }
        }
      }

  player.updateEnd();

  ChunkRenderer::BeginScene(playerCamera);

  if (m_RenderingPaused)
    m_ChunkManager.render(playerCamera);
  else
  {
    m_ChunkManager.updatePlayerChunk(playerCamera.getPosition());

    m_ChunkManager.clean();
    m_ChunkManager.render(playerCamera);
    m_ChunkManager.loadNewChunks(200);
  }

  ChunkRenderer::EndScene();
}

bool World::onKeyPressEvent(Engine::KeyPressEvent& event)
{
  if (event.getKeyCode() == Key::F3)
    m_RenderingPaused = !m_RenderingPaused;

  return false;
}

void World::onEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));
}
