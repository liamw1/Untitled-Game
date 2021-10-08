#include "GMpch.h"
#include "World.h"
#include "ChunkRenderer.h"

/*
  World data
*/
static constexpr int s_RenderDistance = 16;
static constexpr int s_LoadDistance = s_RenderDistance;
static constexpr int s_UnloadDistance = s_LoadDistance;

static std::map<int64_t, Chunk> s_Chunks{};
static std::map<int64_t, HeightMap> s_HeightMaps{};



/*
  Creates a (nearly) unique integer value for a given chunk index.
  Guaranteed to be unique as long as the unload distance < 1024.
  This can be changed to be up to 1024^2 by doubling the bit numbers.
*/
static int64_t createKey(const std::array<int64_t, 3>& chunkIndex)
{
  return chunkIndex[0] % bit(10) + bit(10) * (chunkIndex[1] % bit(10)) + bit(20) * (chunkIndex[2] % bit(10));
}
static int64_t createKey(int64_t chunkX, int64_t chunkZ)
{
  return chunkX % bit(10) + bit(20) * (chunkZ % bit(10));
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

static HeightMap generateHeightMap(int64_t chunkX, int64_t chunkZ)
{
  HeightMap heightMap{};
  heightMap.chunkX = chunkX;
  heightMap.chunkZ = chunkZ;

  for (uint8_t i = 0; i < Chunk::Size(); ++i)
    for (uint8_t k = 0; k < Chunk::Size(); ++k)
    {
      glm::vec2 blockXZ = glm::vec2(chunkX * Chunk::Length() + i * Block::Length(), chunkZ * Chunk::Length() + k * Block::Length());
      float terrainHeight = 10.0f * glm::simplex(blockXZ / 64.0f) + 2.0f * glm::simplex(blockXZ / 32.0f) + glm::simplex(blockXZ / 8.0f);
      heightMap.terrainHeights[i][k] = terrainHeight;
    }
  return heightMap;
}

static void loadNewChunks(const std::array<int64_t, 3>& playerChunkIndex, uint32_t maxNewChunks)
{
  EN_PROFILE_FUNCTION();

  static constexpr int8_t normals[6][3] = { { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1}, { -1, 0, 0}, { 1, 0, 0} };
                                       //       Top        Bottom       North       South         East         West

  uint32_t newChunks = 0;
  for (auto it = s_Chunks.begin(); it != s_Chunks.end(); ++it)
  {
    auto& chunk = it->second;

    // Check if additional chunks can be loaded
    if (!chunk.allNeighborsLoaded())
      for (BlockFace face : BlockFaceIterator())
        if (chunk.getNeighbor(face) == nullptr)
        {
          std::array<int64_t, 3> neighborIndex = chunk.getIndex();
          for (int i = 0; i < 3; ++i)
            neighborIndex[i] += normals[static_cast<uint8_t>(face)][i];

          // If potential chunk is out of load range, do nothing
          if (isInLoadRange(neighborIndex, playerChunkIndex))
          {
            // Create key for hash map
            int64_t key = createKey(neighborIndex);
            EN_ASSERT(s_Chunks.find(key) == s_Chunks.end(), "Chunk is already in map");

            // Create key for height map
            int64_t heightMapKey = createKey(neighborIndex[0], neighborIndex[2]);

            // Generate heightmap is none exists
            if (s_HeightMaps.find(heightMapKey) == s_HeightMaps.end())
              s_HeightMaps[heightMapKey] = generateHeightMap(neighborIndex[0], neighborIndex[2]);

            // Generate chunk
            s_Chunks[key] = std::move(Chunk(neighborIndex));
            Chunk& newChunk = s_Chunks[key];
            newChunk.load(s_HeightMaps[heightMapKey]);

            // Set neighbors in all directions
            for (BlockFace dir : BlockFaceIterator())
            {
              // Index of chunk adjacent to neighboring chunk in direction "dir"
              std::array<int64_t, 3> adjIndex = neighborIndex;
              for (int i = 0; i < 3; ++i)
                adjIndex[i] += normals[static_cast<uint8_t>(dir)][i];

              int64_t adjKey = createKey(adjIndex);
              if (s_Chunks.find(adjKey) != s_Chunks.end())
              {
                Chunk& adjChunk = s_Chunks[adjKey];
                newChunk.setNeighbor(dir, &adjChunk);
                adjChunk.setNeighbor(!dir, &newChunk);
              }
            }

            newChunks++;
          }
        }

    if (newChunks >= maxNewChunks)
      break;
  }
}

static void clean(const std::array<int64_t, 3>& playerChunkIndex)
{
  EN_PROFILE_FUNCTION();

  for (auto it = s_Chunks.begin(); it != s_Chunks.end();)
  {
    auto& chunk = it->second;

    if (isOutOfRange(chunk.getIndex(), playerChunkIndex))
      it = s_Chunks.erase(it);
    else
      it++;
  }

  for (auto it = s_HeightMaps.begin(); it != s_HeightMaps.end();)
  {
    int64_t key = it->first;
    auto& heightMap = it->second;

    if (isOutOfRange({ heightMap.chunkX, playerChunkIndex[1], heightMap.chunkZ }, playerChunkIndex))
      it = s_HeightMaps.erase(it);
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

  std::array<int64_t, 3> playerChunkIndex = Chunk::GetPlayerChunkIndex(initialPosition);

  int64_t heightMapKey = createKey(playerChunkIndex[0], playerChunkIndex[2]);
  s_HeightMaps[heightMapKey] = generateHeightMap(playerChunkIndex[0], playerChunkIndex[2]);

  Chunk newChunk = Chunk(playerChunkIndex);
  newChunk.load(s_HeightMaps[heightMapKey]);
  s_Chunks[createKey(playerChunkIndex)] = std::move(newChunk);
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
  loadNewChunks(playerChunkIndex, 100);

  // EN_INFO("Chunks loaded: {0}", s_Chunks.size());
}
