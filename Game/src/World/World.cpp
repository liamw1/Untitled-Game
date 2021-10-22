#include "GMpch.h"
#include "World.h"
#include "ChunkManager.h"
#include "ChunkRenderer.h"

static bool s_RenderingPaused = false;
static ChunkManager s_ChunkManager{};

void World::Initialize(const glm::vec3& initialPosition)
{
  s_ChunkManager.updatePlayerChunk(Chunk::GetPlayerChunkIndex(initialPosition));

  while (s_ChunkManager.loadNewChunks(10000))
  {
  }
}

void World::ShutDown()
{
}

void World::OnUpdate(const Engine::Camera& playerCamera)
{
  EN_PROFILE_FUNCTION();

  ChunkRenderer::BeginScene(playerCamera);

  if (s_RenderingPaused)
    s_ChunkManager.render(playerCamera);
  else
  {
    s_ChunkManager.updatePlayerChunk(Chunk::GetPlayerChunkIndex(playerCamera.getPosition()));

    s_ChunkManager.clean();
    s_ChunkManager.render(playerCamera);
    s_ChunkManager.loadNewChunks(200);
  }

  ChunkRenderer::EndScene();
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
