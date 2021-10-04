#include "World.h"
#include "ChunkRenderer.h"

/*
  World data
*/
static constexpr int s_RenderDistance = 8;
static constexpr int s_LoadDistance = s_RenderDistance;
static constexpr int s_UnloadDistance = s_LoadDistance;

static std::map<int64_t, Chunk> s_Chunks{};



/*
  Creates a (nearly) unique integer value for a given chunk index.
  Guaranteed to be unique as long as the unload distance < 1024.
  This can be changed to be up to 1024^2 by doubling the bit numbers.
*/
static int64_t createKey(const std::array<int64_t, 3>& chunkIndex)
{
  return chunkIndex[0] % bit(10) + bit(10) * (chunkIndex[1] % bit(10)) + bit(20) * (chunkIndex[2] % bit(10));
}

static bool isOutOfRange(const std::array<int64_t, 3>& chunkIndex, const std::array<int64_t, 3>& playerChunkIndex)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunkIndex[i] - chunkIndex[i]) > s_UnloadDistance)
      return true;
  return false;
}

static bool isInLoadRange(const std::array<int64_t, 3>& chunkIndex, const std::array<int64_t, 3>& playerChunkIndex)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunkIndex[i] - chunkIndex[i]) > s_LoadDistance)
      return false;
  return true;
}

static bool isInRenderRange(const std::array<int64_t, 3>& chunkIndex, const std::array<int64_t, 3>& playerChunkIndex)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunkIndex[i] - chunkIndex[i]) > s_RenderDistance)
      return false;
  return true;
}

static void loadNewChunks(const std::array<int64_t, 3>& playerChunkIndex)
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
          std::array<int64_t, 3> neighborIndex = chunk.getIndex();
          for (int i = 0; i < 3; ++i)
            neighborIndex[i] += normals[face][i];

          // If potential chunk is out of load range, do nothing
          if (isInLoadRange(neighborIndex, playerChunkIndex))
          {
            // Create key for hash map
            int64_t key = createKey(neighborIndex);
            EN_ASSERT(s_Chunks.find(key) == s_Chunks.end(), "Chunk is already in map");

            // Generate chunk
            s_Chunks[key] = std::move(Chunk(neighborIndex));
            Chunk& newChunk = s_Chunks[key];
            newChunk.load(Block::Sand);

            // Set neighbors in all directions
            for (uint8_t dir = (uint8_t)BlockFace::Top; dir <= (uint8_t)BlockFace::West; ++dir)
            {
              // Index of chunk adjacent to neighboring chunk in direction "dir"
              std::array<int64_t, 3> adjIndex = neighborIndex;
              for (int i = 0; i < 3; ++i)
                adjIndex[i] += normals[dir][i];

              int64_t adjKey = createKey(adjIndex);
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

static void clean(const std::array<int64_t, 3>& playerChunkIndex)
{
  EN_PROFILE_FUNCTION();

  for (auto it = s_Chunks.begin(); it != s_Chunks.end();)
  {
    auto& chunk = it->second;

    if (isOutOfRange(chunk.getIndex(), playerChunkIndex))
    {
      chunk.unload();
      it = s_Chunks.erase(it);
    }
    else
      it++;
  }
}

static void render(const std::array<int64_t, 3>& playerChunkIndex)
{
  EN_PROFILE_FUNCTION();

  for (auto it = s_Chunks.begin(); it != s_Chunks.end(); ++it)
  {
    auto& chunk = it->second;

    if (isInRenderRange(chunk.getIndex(), playerChunkIndex) && chunk.allNeighborsLoaded() && !chunk.isEmpty())
    {
      if (!chunk.isMeshGenerated())
        chunk.generateMesh();

      ChunkRenderer::DrawChunk(&chunk);
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
  
  std::array<int64_t, 3> playerChunkIndex = Chunk::GetPlayerChunkIndex(playerPosition);

  clean(playerChunkIndex);
  render(playerChunkIndex);
  loadNewChunks(playerChunkIndex);
}
