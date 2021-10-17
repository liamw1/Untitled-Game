#include "GMpch.h"
#include "World.h"
#include "ChunkManager.h"

static bool s_RenderingPaused = false;
static ChunkManager s_ChunkManager{};

void World::Initialize(const glm::vec3& initialPosition)
{
  Chunk::InitializeIndexBuffer();

  s_ChunkManager.updatePlayerChunk(Chunk::GetPlayerChunkIndex(initialPosition));

  while (s_ChunkManager.loadNewChunks(10000))
  {
  }
}

void World::ShutDown()
{
}

void World::OnUpdate(const glm::vec3& playerPosition)
{
  EN_PROFILE_FUNCTION();

  if (s_RenderingPaused)
    s_ChunkManager.render();
  else
  {
    s_ChunkManager.updatePlayerChunk(Chunk::GetPlayerChunkIndex(playerPosition));

    s_ChunkManager.clean();
    s_ChunkManager.render();
    s_ChunkManager.loadNewChunks(200);
  }
}

static bool onKeyPressEvent(Engine::KeyPressEvent& event)
{
  if (event.getKeyCode() == Key::F3)
    s_RenderingPaused = !s_RenderingPaused;

  return false;
}

void World::OnEvent(Engine::Event& event)
{
  Engine::EventDispatcher dispatcher(event);
  dispatcher.dispatch<Engine::KeyPressEvent>(onKeyPressEvent);
}
