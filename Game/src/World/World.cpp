#include "World.h"

/*
  World data
*/
static int s_RenderDistance = 4;
static int s_LoadDistance = s_RenderDistance + 2;
static int s_PreLoadDistance = s_LoadDistance + 2;

static std::vector<Chunk> s_LoadedChunks{};
static std::array<int64_t, 3> s_LastPlayerChunk{};



static void preLoad(const std::array<int64_t, 3>& playerChunk)
{
  EN_PROFILE_FUNCTION();

  for (int i = playerChunk[0] - s_PreLoadDistance; i <= playerChunk[0] + s_PreLoadDistance; ++i)
    for (int k = playerChunk[2] - s_PreLoadDistance; k <= playerChunk[2] + s_PreLoadDistance; ++k)
    {
      std::array<int64_t, 3> chunkIndices = { i, -1, k };
      bool preLoaded = false;
      for (auto it = s_LoadedChunks.begin(); it != s_LoadedChunks.end(); ++it)
        if (it->getIndices() == chunkIndices)
          preLoaded = true;

      if (!preLoaded)
      {
        Chunk newChunk = Chunk(chunkIndices);
        s_LoadedChunks.push_back(std::move(newChunk));
      }
    }
}

void World::Initialize(const glm::vec3& initialPosition)
{
  std::array<int64_t, 3> playerChunk = Chunk::GetPlayerChunk(initialPosition);
  preLoad(playerChunk);
}

void World::ShutDown()
{
}

void World::OnUpdate(const glm::vec3& playerPosition)
{
  EN_PROFILE_FUNCTION();
  
  std::array<int64_t, 3> playerChunk = Chunk::GetPlayerChunk(playerPosition);
  if (playerChunk != s_LastPlayerChunk)
  {
    preLoad(playerChunk);
    s_LastPlayerChunk = playerChunk;
  }

  for (auto it = s_LoadedChunks.begin(); it != s_LoadedChunks.end();)
  {
    std::array<int64_t, 3> distance = { abs(playerChunk[0] - it->getIndices()[0]), abs(playerChunk[1] - it->getIndices()[1]) , abs(playerChunk[2] - it->getIndices()[2]) };
    if (distance[0] > s_PreLoadDistance || distance[2] > s_PreLoadDistance)
    {
      it = s_LoadedChunks.erase(it);
    }
    else if (!it->isLoaded() && distance[0] < s_LoadDistance && distance[2] < s_LoadDistance)
    {
      it->load(Block::Sand);
      it++;
    }
    else if (distance[0] < s_RenderDistance && distance[2] < s_RenderDistance && it->isLoaded())
    {
      ChunkRenderer::DrawChunk(*it);
      it++;
    }
    else
      it++;
  }
}
