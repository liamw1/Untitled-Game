#include "World.h"

/*
  World data
*/
static constexpr int s_RenderDistance = 4;
static constexpr int s_LoadDistance = s_RenderDistance + 4;
static constexpr int s_PreLoadDistance = s_LoadDistance + 8;

static std::array<int64_t, 3> s_LastPlayerChunk{};
static std::unordered_map<int64_t, Chunk> s_Chunks{};


/*
  Creates a (nearly) unique integer value for a given set of chunk indices.
  Guaranteed to be unique as long as the preload distance < 1024.
  This can be changed to be up to 1024^2 by doubling the bit numbers.
*/
static int64_t createKey(const std::array<int64_t, 3>& chunkIndices)
{
  return chunkIndices[0] % bit(10) + bit(10) * (chunkIndices[1] % bit(10)) + bit(20) * (chunkIndices[2] % bit(10));
}

static void preLoad(const std::array<int64_t, 3>& playerChunk)
{
  EN_PROFILE_FUNCTION();

  for (int64_t i = playerChunk[0] - s_PreLoadDistance; i <= playerChunk[0] + s_PreLoadDistance; ++i)
    for (int64_t k = playerChunk[2] - s_PreLoadDistance; k <= playerChunk[2] + s_PreLoadDistance; ++k)
    {
      std::array<int64_t, 3> chunkIndices = { i, -1, k };
      int64_t key = createKey(chunkIndices);
      if (s_Chunks.find(key) == s_Chunks.end())
        s_Chunks[key] = std::move(Chunk(chunkIndices));
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

  for (auto it = s_Chunks.begin(); it != s_Chunks.end();)
  {
    auto& chunk = it->second;
    std::array<int64_t, 3> distance = { abs(playerChunk[0] - chunk.getIndices()[0]), abs(playerChunk[1] - chunk.getIndices()[1]) , abs(playerChunk[2] - chunk.getIndices()[2]) };
    if (distance[0] > s_PreLoadDistance || distance[2] > s_PreLoadDistance)
    {
      it = s_Chunks.erase(it);
    }
    else if (!chunk.isLoaded() && distance[0] < s_LoadDistance && distance[2] < s_LoadDistance)
    {
      chunk.load(Block::Sand);
      it++;
    }
    else if (distance[0] < s_RenderDistance && distance[2] < s_RenderDistance && chunk.isLoaded())
    {
      ChunkRenderer::DrawChunk(chunk);
      it++;
    }
    else
      it++;
  }
}
