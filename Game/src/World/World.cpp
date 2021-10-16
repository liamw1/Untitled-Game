#include "GMpch.h"
#include "World.h"
#include "ChunkManager.h"

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

  s_ChunkManager.updatePlayerChunk(Chunk::GetPlayerChunkIndex(playerPosition));

  s_ChunkManager.clean();
  s_ChunkManager.render();
  s_ChunkManager.loadNewChunks(200);
}
