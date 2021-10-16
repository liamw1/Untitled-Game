#include "GMpch.h"
#include "World.h"
#include "ChunkRenderer.h"
#include <llvm/ADT/DenseMap.h>

/*
  World data
*/
enum class MapType : uint8_t
{
  Empty = 0,
  Boundary,
  Renderable,

  First = Empty, Last = Renderable
};

class MapTypeIterator
{
public:
  MapTypeIterator(const MapType mapType)
    : value(static_cast<uint8_t>(mapType))
  {
  }
  MapTypeIterator()
    : value(static_cast<uint8_t>(MapType::First))
  {
  }

  MapTypeIterator& operator++()
  {
    ++value;
    return *this;
  }
  MapType operator*() { return static_cast<MapType>(value); }
  bool operator!=(const MapTypeIterator& iter) { return value != iter.value; }

  MapTypeIterator begin() { return *this; }
  MapTypeIterator end()
  {
    static const MapTypeIterator endIter = ++MapTypeIterator(MapType::Last);
    return endIter;
  }

private:
  uint8_t value;
};

static constexpr int s_RenderDistance = 16;
static constexpr int s_LoadDistance = s_RenderDistance + 2;
static constexpr int s_UnloadDistance = s_LoadDistance;

static llvm::DenseMap<int64_t, HeightMap> s_HeightMaps{};

// Idea: Maps only should store pointers to chunks, which reside in a single array/vector
static std::array<llvm::DenseMap<int64_t, Chunk>, 3> s_Chunks;
static llvm::DenseMap<int64_t, Chunk>& s_EmptyChunks = s_Chunks[static_cast<uint8_t>(MapType::Empty)];
static llvm::DenseMap<int64_t, Chunk>& s_BoundaryChunks = s_Chunks[static_cast<uint8_t>(MapType::Boundary)];
static llvm::DenseMap<int64_t, Chunk>& s_RenderableChunks = s_Chunks[static_cast<uint8_t>(MapType::Renderable)];


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
      float terrainHeight = 30.0f * glm::simplex(blockXY / 256.0f) + 10.0f * glm::simplex(blockXY / 64.0f) + glm::simplex(blockXY / 8.0f);
      heightMap.terrainHeights[i][j] = terrainHeight;
    }
  return heightMap;
}

static bool loadNewChunks(const ChunkIndex& playerChunkIndex, uint32_t maxNewChunks)
{
  EN_PROFILE_FUNCTION();

  static constexpr int8_t normals[6][3] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  // Load First chunk if none exist
  bool noChunksLoaded = true;
  for (MapType mapType : MapTypeIterator())
  {
    const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

    if (s_Chunks[mapTypeID].size() > 0)
      noChunksLoaded = false;
  }
  if (noChunksLoaded)
  {
    int64_t heightMapKey = createHeightMapKey(playerChunkIndex[0], playerChunkIndex[1]);
    s_HeightMaps[heightMapKey] = generateHeightMap(playerChunkIndex[0], playerChunkIndex[1]);

    int64_t key = createKey(playerChunkIndex);
    s_BoundaryChunks.try_emplace(key, playerChunkIndex);

    Chunk& playerChunk = s_BoundaryChunks[key];
    playerChunk.load(s_HeightMaps[heightMapKey]);
  }

  // Find new chunks to generate
  std::vector<ChunkIndex> newChunks{};
  std::vector<ChunkIndex> chunksToMove{};
  for (auto& pair : s_BoundaryChunks)
  {
    const Chunk& chunk = pair.second;

    bool isOnBoundary = false;
    for (BlockFace face : BlockFaceIterator())
      if (chunk.getNeighbor(face) == nullptr && !chunk.isFaceOpaque(face))
      {
        isOnBoundary = true;

        // Store index of potential new chunk
        ChunkIndex neighborIndex = chunk.getIndex();
        for (int i = 0; i < 3; ++i)
          neighborIndex[i] += normals[static_cast<uint8_t>(face)][i];

        // If potential chunk is out of load range, skip it
        if (!isInLoadRange(neighborIndex, playerChunkIndex))
          continue;

        newChunks.push_back(neighborIndex);
      }

    if (!isOnBoundary)
      chunksToMove.push_back(chunk.getIndex());

    if (newChunks.size() >= maxNewChunks)
      break;
  }

  // Move chunks out of s_BoundaryChunks when all their neighbors are accounted for
  for (int i = 0; i < chunksToMove.size(); ++i)
  {
    int64_t key = createKey(chunksToMove[i]);
    Chunk& chunk = s_BoundaryChunks[key];

    MapType destination = chunk.isEmpty() ? MapType::Empty : MapType::Renderable;
    auto insertionResult = s_Chunks[static_cast<uint8_t>(destination)].insert({ key, std::move(chunk) });
    bool insertionSuccess = insertionResult.second;
    EN_ASSERT(insertionSuccess, "Chunk is already in map!");

    s_BoundaryChunks.erase(key);
  }

  // Added new chunks to s_BoundaryChunks
  for (int i = 0; i < newChunks.size(); ++i)
  {
    const ChunkIndex newChunkIndex = newChunks[i];

    // Create key for hash map.  If chunk is already in map, skip it
    int64_t key = createKey(newChunkIndex);
    for (MapType mapType : MapTypeIterator())
    {
      const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

      if (s_Chunks[mapTypeID].find(key) != s_Chunks[mapTypeID].end())
        continue;
    }

    // Generate heightmap is none exists
    int64_t heightMapKey = createHeightMapKey(newChunkIndex[0], newChunkIndex[1]);
    if (s_HeightMaps.find(heightMapKey) == s_HeightMaps.end())
      s_HeightMaps[heightMapKey] = generateHeightMap(newChunkIndex[0], newChunkIndex[1]);

    // Generate chunk
    auto insertionResult = s_BoundaryChunks.try_emplace(key, newChunkIndex);
    bool insertionSuccess = insertionResult.second;
    EN_ASSERT(insertionSuccess, "Chunk is already in map!");

    Chunk& newChunk = s_BoundaryChunks[key];
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
      for (MapType mapType : MapTypeIterator())
      {
        const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

        if (s_Chunks[mapTypeID].find(adjKey) != s_Chunks[mapTypeID].end())
        {
          Chunk& adjChunk = s_Chunks[mapTypeID][adjKey];
          newChunk.setNeighbor(dir, &adjChunk);
          adjChunk.setNeighbor(!dir, &newChunk);

          break;
        }
      }
    }
  }

  return newChunks.size() > 0;
}

static void clean(const ChunkIndex& playerChunkIndex)
{
  EN_PROFILE_FUNCTION();

  // Destroy chunks outside of unload range
  std::vector<std::pair<MapType, int64_t>> chunksToRemove{};
  for (MapType mapType : MapTypeIterator())
  {
    const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

    for (auto& pair : s_Chunks[mapTypeID])
    {
      const Chunk& chunk = pair.second;
      const ChunkIndex chunkIndex = chunk.getIndex();

      if (isOutOfRange(chunkIndex, playerChunkIndex))
        chunksToRemove.emplace_back(mapType, createKey(chunkIndex));
    }
  }
  for (int i = 0; i < chunksToRemove.size(); ++i)
  {
    const uint8_t mapTypeID = static_cast<uint8_t>(chunksToRemove[i].first);
    const int64_t& key = chunksToRemove[i].second;

    // NOTE: Should probably move neighbors to s_BoundaryChunks before erasing, but it works fine now so whatevs
    bool eraseSuccessful = s_Chunks[mapTypeID].erase(key);
    EN_ASSERT(eraseSuccessful, "Chunk is not in map!");
  }

  // Destroy heightmaps outside of unload range
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

  for (auto& pair : s_RenderableChunks)
  {
    Chunk& chunk = pair.second;

    if (isInRenderRange(chunk.getIndex(), playerChunkIndex))
    {
      chunk.update();
      ChunkRenderer::DrawChunk(chunk);
    }
  }
}

void World::Initialize(const glm::vec3& initialPosition)
{
  Chunk::InitializeIndexBuffer();

  ChunkIndex playerChunkIndex = Chunk::GetPlayerChunkIndex(initialPosition);

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
}
