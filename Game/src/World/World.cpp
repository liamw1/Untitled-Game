#include "GMpch.h"
#include "World.h"
#include "ChunkRenderer.h"
#include <llvm/ADT/DenseMap.h>

/*
  World data
*/
static constexpr int s_RenderDistance = 16;
static constexpr int s_LoadDistance = s_RenderDistance + 2;
static constexpr int s_UnloadDistance = s_LoadDistance;

static llvm::DenseMap<int64_t, Chunk> s_Chunks{};
static llvm::DenseMap<int64_t, HeightMap> s_HeightMaps{};



/*
  Creates a (nearly) unique integer value for a given chunk index.
  Guaranteed to be unique as long as the unload distance < 1024.
  This can be changed to be up to 1024^2 by doubling the bit numbers.
*/
static int64_t createKey(const ChunkIndex& chunkIndex)
{
  return chunkIndex[0] % bit(10) + bit(10) * (chunkIndex[1] % bit(10)) + bit(20) * (chunkIndex[2] % bit(10));
}
static int64_t createHeightMapKey(int64_t chunkX, int64_t chunkY)
{
  return chunkX % bit(10) + bit(10) * (chunkY % bit(10));
}

static bool isOutOfRange(const ChunkIndex& chunkIndex, const ChunkIndex& playerChunkIndex)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunkIndex[i] - chunkIndex[i]) > s_UnloadDistance)
      return true;
  return false;
}

static bool isInLoadRange(const ChunkIndex& chunkIndex, const ChunkIndex& playerChunkIndex)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunkIndex[i] - chunkIndex[i]) > s_LoadDistance)
      return false;
  return true;
}

static bool isInRenderRange(const ChunkIndex& chunkIndex, const ChunkIndex& playerChunkIndex)
{
  for (int i = 0; i < 3; ++i)
    if (abs(playerChunkIndex[i] - chunkIndex[i]) > s_RenderDistance)
      return false;
  return true;
}

static HeightMap generateHeightMap(int64_t chunkX, int64_t chunkY)
{
  HeightMap heightMap{};
  heightMap.chunkX = chunkX;
  heightMap.chunkY = chunkY;

  for (uint8_t i = 0; i < Chunk::Size(); ++i)
    for (uint8_t j = 0; j < Chunk::Size(); ++j)
    {
      glm::vec2 blockXY = glm::vec2(chunkX * Chunk::Length() + i * Block::Length(), chunkY * Chunk::Length() + j * Block::Length());
      float terrainHeight = 10.0f * glm::simplex(blockXY / 64.0f) + 2.0f * glm::simplex(blockXY / 32.0f) + glm::simplex(blockXY / 8.0f);
      heightMap.terrainHeights[i][j] = terrainHeight;
    }
  return heightMap;
}

static bool loadNewChunks(const ChunkIndex& playerChunkIndex, uint32_t maxNewChunks)
{
  EN_PROFILE_FUNCTION();

  static constexpr int8_t normals[6][3] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  // Find new chunks to generate
  std::vector<ChunkIndex> newChunks{};
  for (auto& pair : s_Chunks)
  {
    const Chunk& chunk = pair.second;

    for (BlockFace face : BlockFaceIterator())
      if (chunk.getNeighbor(face) == nullptr && !chunk.isFaceOpaque(face))
      {
        // Store index of potential new chunk
        ChunkIndex neighborIndex = chunk.getIndex();
        for (int i = 0; i < 3; ++i)
          neighborIndex[i] += normals[static_cast<uint8_t>(face)][i];

        // If potential chunk is out of load range, skip it
        if (!isInLoadRange(neighborIndex, playerChunkIndex))
          continue;

        newChunks.push_back(neighborIndex);
      }

    if (newChunks.size() >= maxNewChunks)
      break;
  }

  // Added new chunks to map
  for (int i = 0; i < newChunks.size(); ++i)
  {
    ChunkIndex newChunkIndex = newChunks[i];

    // Create key for hash map.  If chunk is already in map, skip it
    int64_t key = createKey(newChunkIndex);
    if (s_Chunks.find(key) != s_Chunks.end())
      continue;

    // Generate heightmap is none exists
    int64_t heightMapKey = createHeightMapKey(newChunkIndex[0], newChunkIndex[1]);
    if (s_HeightMaps.find(heightMapKey) == s_HeightMaps.end())
      s_HeightMaps[heightMapKey] = generateHeightMap(newChunkIndex[0], newChunkIndex[1]);

    // Generate chunk
    auto insertionResult = s_Chunks.try_emplace(key, newChunkIndex);  // Construct chunk in-place
    bool insertionSuccess = insertionResult.second;
    EN_ASSERT(insertionSuccess, "Chunk is already in map!");

    Chunk& newChunk = s_Chunks[key];
    newChunk.load(s_HeightMaps[heightMapKey]);

    // Set neighbors in all directions
    for (BlockFace dir : BlockFaceIterator())
    {
      // Store index of chunk adjacent to new chunk in direction "dir"
      ChunkIndex adjIndex = newChunkIndex;
      for (int i = 0; i < 3; ++i)
        adjIndex[i] += normals[static_cast<uint8_t>(dir)][i];

      // Find and add neighbors (if they exist) to new chunk
      int64_t adjKey = createKey(adjIndex);
      if (s_Chunks.find(adjKey) != s_Chunks.end())
      {
        Chunk& adjChunk = s_Chunks[adjKey];
        newChunk.setNeighbor(dir, &adjChunk);
        adjChunk.setNeighbor(!dir, &newChunk);
      }
    }
  }

  return newChunks.size() > 0;
}

static void clean(const ChunkIndex& playerChunkIndex)
{
  EN_PROFILE_FUNCTION();

  std::vector<int64_t> chunksToRemove{};
  for (auto& pair : s_Chunks)
  {
    const Chunk& chunk = pair.second;
    const ChunkIndex chunkIndex = chunk.getIndex();

    if (isOutOfRange(chunkIndex, playerChunkIndex))
      chunksToRemove.push_back(createKey(chunkIndex));
  }
  for (int i = 0; i < chunksToRemove.size(); ++i)
    s_Chunks.erase(chunksToRemove[i]);

  std::vector<int64_t> heightMapsToRemove{};
  for (auto& pair : s_HeightMaps)
  {
    const HeightMap& heightMap = pair.second;

    if (isOutOfRange({ heightMap.chunkX, heightMap.chunkY, playerChunkIndex[2] }, playerChunkIndex))
      heightMapsToRemove.push_back(createHeightMapKey(heightMap.chunkX, heightMap.chunkY));
  }
  for (int i = 0; i < heightMapsToRemove.size(); ++i)
    s_HeightMaps.erase(heightMapsToRemove[i]);
}

static void render(const ChunkIndex& playerChunkIndex)
{
  EN_PROFILE_FUNCTION();

  for (auto& pair : s_Chunks)
  {
    Chunk& chunk = pair.second;

    if (!chunk.isEmpty() && isInRenderRange(chunk.getIndex(), playerChunkIndex))
    {
      chunk.onUpdate();
      ChunkRenderer::DrawChunk(&chunk);
    }
  }
}

void World::Initialize(const glm::vec3& initialPosition)
{
  Chunk::InitializeIndexBuffer();

  ChunkIndex playerChunkIndex = Chunk::GetPlayerChunkIndex(initialPosition);

  int64_t heightMapKey = createHeightMapKey(playerChunkIndex[0], playerChunkIndex[1]);
  s_HeightMaps[heightMapKey] = generateHeightMap(playerChunkIndex[0], playerChunkIndex[1]);

  int64_t key = createKey(playerChunkIndex);
  s_Chunks.try_emplace(key, playerChunkIndex);
  Chunk& playerChunk = s_Chunks[key];
  playerChunk.load(s_HeightMaps[heightMapKey]);

  while (loadNewChunks(playerChunkIndex, 10000))
  {
  }
}

void World::ShutDown()
{
}

void World::OnUpdate(const glm::vec3& playerPosition)
{
  EN_PROFILE_FUNCTION();
  
  ChunkIndex playerChunkIndex = Chunk::GetPlayerChunkIndex(playerPosition);

  clean(playerChunkIndex);
  render(playerChunkIndex);
  loadNewChunks(playerChunkIndex, 200);

  // EN_INFO("Chunks loaded: {0}", s_Chunks.size());
}
