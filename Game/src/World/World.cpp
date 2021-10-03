#include "World.h"
#include "ChunkRenderer.h"

/*
  World data
*/
static constexpr int s_RenderDistance = 4;
static constexpr int s_LoadDistance = s_RenderDistance;
static constexpr int s_UnloadDistance = s_LoadDistance;

static std::array<int64_t, 3> s_LastPlayerChunk{};
static std::unordered_map<int64_t, Chunk> s_Chunks{};



/*
  Creates a (nearly) unique integer value for a given set of chunk indices.
  Guaranteed to be unique as long as the unload distance < 1024.
  This can be changed to be up to 1024^2 by doubling the bit numbers.
*/
static int64_t createKey(const std::array<int64_t, 3>& chunkIndices)
{
  return chunkIndices[0] % bit(10) + bit(10) * (chunkIndices[1] % bit(10)) + bit(20) * (chunkIndices[2] % bit(10));
}

static bool isOutOfRange(const std::array<int64_t, 3>& chunk, const std::array<int64_t, 3>& playerChunk)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunk[i] - chunk[i]) > s_UnloadDistance)
      return true;
  return false;
}

static bool isInLoadRange(const std::array<int64_t, 3>& chunk, const std::array<int64_t, 3>& playerChunk)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunk[i] - chunk[i]) > s_LoadDistance)
      return false;
  return true;
}

static bool isInRenderRange(const std::array<int64_t, 3>& chunk, const std::array<int64_t, 3>& playerChunk)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunk[i] - chunk[i]) > s_RenderDistance)
      return false;
  return true;
}

static void LoadNewChunks(const std::array<int64_t, 3>& playerChunk)
{
  EN_PROFILE_FUNCTION();

  static constexpr int8_t normals[6][3] = { { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1}, { -1, 0, 0}, { 1, 0, 0} };
                                       //       Top        Bottom       North       South         East         West

  for (auto it = s_Chunks.begin(); it != s_Chunks.end(); ++it)
  {
    auto& chunk = it->second;

    // Check if additional chunks can be loaded
    if (!chunk.allNeighborsLoaded())
      for (uint8_t face = (uint8_t)BlockFace::Top; face <= (uint8_t)BlockFace::West; ++face)
        if (chunk.getNeighbor(face) == nullptr)
        {
          std::array<int64_t, 3> neighborIndices = chunk.getIndices();
          for (int i = 0; i < 3; ++i)
            neighborIndices[i] += normals[face][i];

          // If potential chunk is out of load range, do nothing
          if (isInLoadRange(neighborIndices, playerChunk))
          {
            // Create key for hash map
            int64_t key = createKey(neighborIndices);
            EN_ASSERT(s_Chunks.find(key) == s_Chunks.end(), "Chunk is already in map");

            // Generate chunk
            s_Chunks[key] = std::move(Chunk(neighborIndices));
            Chunk& newChunk = s_Chunks[key];
            newChunk.load(Block::Sand);

            // Set neighbors in all directions
            for (uint8_t dir = (uint8_t)BlockFace::Top; dir <= (uint8_t)BlockFace::West; ++dir)
            {
              // Indices of chunk adjacent to neighboring chunk in direction "dir"
              std::array<int64_t, 3> adjIndices = neighborIndices;
              for (int i = 0; i < 3; ++i)
                adjIndices[i] += normals[dir][i];

              int64_t adjKey = createKey(adjIndices);
              if (s_Chunks.find(adjKey) != s_Chunks.end())
              {
                uint8_t oppDir = dir % 2 == 0 ? dir + 1 : dir - 1;

                Chunk& adjChunk = s_Chunks[adjKey];
                newChunk.setNeighbor(dir, &adjChunk);
                adjChunk.setNeighbor(oppDir, &newChunk);
              }
            }
          }
        }
  }
}

void World::Initialize(const glm::vec3& initialPosition)
{
  Chunk::InitializeIndexBuffer();

  Chunk newChunk = Chunk({ 0, -1, 0 });
  newChunk.load(Block::Sand);
  s_Chunks[createKey({ 0, -1, 0 })] = std::move(newChunk);
}

void World::ShutDown()
{
}

void World::OnUpdate(const glm::vec3& playerPosition)
{
  EN_PROFILE_FUNCTION();
  
  std::array<int64_t, 3> playerChunk = Chunk::GetPlayerChunk(playerPosition);
  // if (playerChunk != s_LastPlayerChunk)
  {
    LoadNewChunks(playerChunk);
    s_LastPlayerChunk = playerChunk;
  }

  int count = 0;
  for (auto it = s_Chunks.begin(); it != s_Chunks.end();)
  {
    auto& chunk = it->second;

    if (isOutOfRange(chunk.getIndices(), playerChunk))
    {
      chunk.unload();
      it = s_Chunks.erase(it);
    }
    else if (isInRenderRange(chunk.getIndices(), playerChunk) && chunk.allNeighborsLoaded() && !chunk.isEmpty())
    {
      if (!chunk.isMeshGenerated())
        chunk.generateMesh();

      ChunkRenderer::DrawChunk(&chunk);
      it++;
    }
    else
      it++;
  }
}
