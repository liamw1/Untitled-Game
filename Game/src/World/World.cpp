#include "World.h"

/*
  World data
*/
static int32_t s_RenderDistance = 4;
static float s_RenderRadius = s_RenderDistance * Chunk::Length();
static std::vector<Chunk> s_LoadedChunks{};

void World::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_LoadedChunks.reserve(4 * (int64_t)s_RenderDistance * s_RenderDistance);
  for (int32_t i = 0; i < 2 * s_RenderDistance; ++i)
    for (int32_t j = 0; j < 2 * s_RenderDistance; ++j)
    {
      Chunk newChunk = Chunk({ i - (int64_t)s_RenderDistance, -1, j - (int64_t)s_RenderDistance });
      newChunk.load(Block::Sand);
      s_LoadedChunks.push_back(std::move(newChunk));
    }
}

void World::ShutDown()
{
}

void World::OnUpdate(const glm::vec3& playerPosition)
{
  EN_PROFILE_FUNCTION();
  
  for (auto it = s_LoadedChunks.begin(); it != s_LoadedChunks.end();)
  {
    float xzDistance = sqrt((playerPosition.x - it->chunkCenter().x) * (playerPosition.x - it->chunkCenter().x) + (playerPosition.z - it->chunkCenter().z) * (playerPosition.z - it->chunkCenter().z));
    if (xzDistance > s_RenderRadius)
    {
      it = s_LoadedChunks.erase(it);
    }
    else
    {
      ChunkRenderer::DrawChunk(*it);
      ++it;
    }
  }
}
