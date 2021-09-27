#include "World.h"

/*
  World data
*/
static int32_t s_RenderDistance = 4;
static std::vector<Chunk> s_LoadedChunks{};

void World::Initialize()
{
  EN_PROFILE_FUNCTION();

  s_LoadedChunks.reserve((int64_t)s_RenderDistance * s_RenderDistance);
  for (int32_t i = 0; i < s_RenderDistance; ++i)
    for (int32_t j = 0; j < s_RenderDistance; ++j)
    {
      Chunk newChunk = Chunk({ i - (int64_t)s_RenderDistance / 2, -1, j - (int64_t)s_RenderDistance / 2 });
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

  for (auto it = s_LoadedChunks.begin(); it != s_LoadedChunks.end(); ++it)
    ChunkRenderer::DrawChunk(*it);
}
