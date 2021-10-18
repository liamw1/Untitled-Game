#include "GMpch.h"
#include "ChunkManager.h"
#include "ChunkRenderer.h"

ChunkManager::ChunkManager()
{
  m_ChunkArray = Engine::CreateUnique<Chunk[]>(s_TotalPossibleChunks);

  m_OpenChunkSlots.reserve(s_TotalPossibleChunks);
  for (int i = 0; i < s_TotalPossibleChunks; ++i)
    m_OpenChunkSlots.push_back(i);
}

void ChunkManager::clean()
{
  EN_PROFILE_FUNCTION();

  // Destroy boundary chunks outside of unload range
  std::vector<Chunk*> chunksToRemove{};
  for (auto& pair : m_BoundaryChunks)
  {
    Chunk* chunk = pair.second;

    if (isOutOfRange(chunk->getIndex()))
      chunksToRemove.push_back(chunk);
  }
  for (int i = 0; i < chunksToRemove.size(); ++i)
    unloadChunk(chunksToRemove[i]);

  // Destroy heightmaps outside of unload range
  std::vector<int64_t> heightMapsToRemove{};
  for (auto& pair : m_HeightMaps)
  {
    const HeightMap& heightMap = pair.second;

    if (isOutOfRange({ heightMap.chunkX, heightMap.chunkY, m_PlayerChunkIndex[2] }))
      heightMapsToRemove.push_back(createHeightMapKey(heightMap.chunkX, heightMap.chunkY));
  }
  for (int i = 0; i < heightMapsToRemove.size(); ++i)
    m_HeightMaps.erase(heightMapsToRemove[i]);
}

void ChunkManager::render(const Engine::Camera& playerCamera)
{
  EN_PROFILE_FUNCTION();

  for (auto& pair : m_RenderableChunks)
  {
    Chunk* chunk = pair.second;

    if (isInRenderRange(chunk->getIndex()))
    {
      chunk->update();

      if (isInViewFrustum(chunk, playerCamera))
        ChunkRenderer::DrawChunk(chunk);
    }
  }
}

bool ChunkManager::loadNewChunks(uint32_t maxNewChunks)
{
  EN_PROFILE_FUNCTION();

  static constexpr int8_t normals[6][3] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  // Load First chunk if none exist
  bool noChunksLoaded = true;
  for (MapType mapType : MapTypeIterator())
  {
    const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

    if (m_Chunks[mapTypeID].size() > 0)
      noChunksLoaded = false;
  }
  if (noChunksLoaded)
    loadNewChunk(m_PlayerChunkIndex);

  // Find new chunks to generate
  std::vector<ChunkIndex> newChunks{};
  std::vector<Chunk*> chunksToRecategorize{};
  for (auto& pair : m_BoundaryChunks)
  {
    Chunk* chunk = pair.second;

    bool isOnBoundary = false;
    for (BlockFace face : BlockFaceIterator())
      if (chunk->getNeighbor(face) == nullptr && !chunk->isFaceOpaque(face))
      {
        isOnBoundary = true;

        // Store index of potential new chunk
        ChunkIndex neighborIndex = chunk->getIndex();
        for (int i = 0; i < 3; ++i)
          neighborIndex[i] += normals[static_cast<uint8_t>(face)][i];

        // If potential chunk is out of load range, skip it
        if (!isInLoadRange(neighborIndex))
          continue;

        newChunks.push_back(neighborIndex);
      }

    if (!isOnBoundary)
      chunksToRecategorize.push_back(chunk);

    if (newChunks.size() >= maxNewChunks)
      break;
  }

  // Move chunk pointers out of m_BoundaryChunks when all their neighbors are accounted for
  for (int i = 0; i < chunksToRecategorize.size(); ++i)
  {
    Chunk* chunk = chunksToRecategorize[i];

    MapType destination = chunk->isEmpty() ? MapType::Empty : MapType::Renderable;
    moveToMap(chunk, MapType::Boundary, destination);
  }

  // Added new chunks to m_BoundaryChunks
  for (int i = 0; i < newChunks.size(); ++i)
  {
    const ChunkIndex newChunkIndex = newChunks[i];
    int64_t key = createKey(newChunkIndex);

    // Create key for hash map.  If chunk is already in a map, skip it
    bool isInMap = false;
    for (MapType mapType : MapTypeIterator())
    {
      const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

      if (m_Chunks[mapTypeID].find(key) != m_Chunks[mapTypeID].end())
      {
        isInMap = true;
        break;
      }
    }
    if (isInMap)
      continue;

    // Generate Chunk
    Chunk* newChunk = loadNewChunk(newChunkIndex);

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

        if (m_Chunks[mapTypeID].find(adjKey) != m_Chunks[mapTypeID].end())
        {
          Chunk* adjChunk = m_Chunks[mapTypeID][adjKey];
          newChunk->setNeighbor(dir, adjChunk);
          adjChunk->setNeighbor(!dir, newChunk);

          break;
        }
      }
    }
  }

  return newChunks.size() > 0;
}

int64_t ChunkManager::createKey(const ChunkIndex& chunkIndex) const
{
  return chunkIndex[0] % bit(10) + bit(10) * (chunkIndex[1] % bit(10)) + bit(20) * (chunkIndex[2] % bit(10));
}

int64_t ChunkManager::createHeightMapKey(int64_t chunkX, int64_t chunkY) const
{
  return chunkX % bit(10) + bit(10) * (chunkY % bit(10));
}

bool ChunkManager::isOutOfRange(const ChunkIndex& chunkIndex) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(m_PlayerChunkIndex[i] - chunkIndex[i]) > s_UnloadDistance)
      return true;
  return false;
}

bool ChunkManager::isInLoadRange(const ChunkIndex& chunkIndex) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(m_PlayerChunkIndex[i] - chunkIndex[i]) > s_LoadDistance)
      return false;
  return true;
}

bool ChunkManager::isInRenderRange(const ChunkIndex& chunkIndex) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(m_PlayerChunkIndex[i] - chunkIndex[i]) > s_RenderDistance)
      return false;
  return true;
}

bool ChunkManager::isInViewFrustum(const Chunk* chunk, const Engine::Camera& playerCamera) const
{
  /*
    Right now we check that each corner is inside frustum, but really we should check whether
    each chunk face collides with any of the frustum planes.  Current method results in 
    chunks not being rendered on screen when player is close to them.
  */

  static constexpr glm::vec3 cornerOffsets[8] = { glm::vec3(0.0f,            0.0f,            0.0f           ),
                                                  glm::vec3(Chunk::Length(), 0.0f,            0.0f           ),
                                                  glm::vec3(0.0f,            Chunk::Length(), 0.0f           ),
                                                  glm::vec3(Chunk::Length(), Chunk::Length(), 0.0f           ),
                                                  glm::vec3(0.0f,            0.0f,            Chunk::Length()),
                                                  glm::vec3(Chunk::Length(), 0.0f,            Chunk::Length()),
                                                  glm::vec3(0.0f,            Chunk::Length(), Chunk::Length()),
                                                  glm::vec3(Chunk::Length(), Chunk::Length(), Chunk::Length()) };

  for (int i = 0; i < 8; ++i)
  {
    glm::vec4 vPrime = playerCamera.getProjectionMatrix() * playerCamera.getViewMatrix() * glm::vec4(chunk->position() + cornerOffsets[i], 1.0f);

    if (abs(vPrime[0]) < vPrime[3] && abs(vPrime[1]) < vPrime[3] && abs(vPrime[2]) < vPrime[3])
      return true;
  }
  return false;
}

HeightMap ChunkManager::generateHeightMap(int64_t chunkX, int64_t chunkY)
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

Chunk* ChunkManager::loadNewChunk(const ChunkIndex& chunkIndex)
{
  EN_ASSERT(m_OpenChunkSlots.size() > 0, "All chunks slots are full!");

  // Grab first open chunk slot
  int chunkSlot = m_OpenChunkSlots.back();
  m_OpenChunkSlots.pop_back();

  // Generate heightmap is none exists
  int64_t heightMapKey = createHeightMapKey(chunkIndex[0], chunkIndex[1]);
  if (m_HeightMaps.find(heightMapKey) == m_HeightMaps.end())
    m_HeightMaps[heightMapKey] = generateHeightMap(chunkIndex[0], chunkIndex[1]);

  // Insert chunk into array and load it
  m_ChunkArray[chunkSlot] = std::move(Chunk(chunkIndex));
  Chunk* newChunk = &m_ChunkArray[chunkSlot];
  newChunk->load(m_HeightMaps[heightMapKey]);

  // Insert chunk pointer into boundary chunk map
  addToMap(newChunk, MapType::Boundary);

  return newChunk;
}

void ChunkManager::unloadChunk(Chunk* chunk)
{
  EN_ASSERT(m_BoundaryChunks.find(createKey(chunk->getIndex())) != m_BoundaryChunks.end(), "Chunk is not in boundary chunk map!");

  // Move neighbors to m_BoundaryChunks
  for (BlockFace face : BlockFaceIterator())
  {
    Chunk* chunkNeighbor = chunk->getNeighbor(face);

    // If neighbor does not exist, skip
    if (chunkNeighbor == nullptr)
      continue;

    // Check if chunk is in m_BoundaryChunk.  If so, skip it
    if (m_BoundaryChunks.find(createKey(chunkNeighbor->getIndex())) != m_BoundaryChunks.end())
      continue;

    // Move chunk pointer to m_BoundaryChunks
    MapType source = chunkNeighbor->isEmpty() ? MapType::Empty : MapType::Renderable;
    EN_ASSERT(m_Chunks[static_cast<uint8_t>(source)].find(createKey(chunkNeighbor->getIndex())) != m_Chunks[static_cast<uint8_t>(source)].end(), "Chunk is not in correct map!");
    moveToMap(chunkNeighbor, source, MapType::Boundary);
  }

  // Remove chunk pointer from m_BoundaryChunks
  int64_t key = createKey(chunk->getIndex());
  bool eraseSuccessful = m_BoundaryChunks.erase(key);
  EN_ASSERT(eraseSuccessful, "Chunk is not in map!");

  // Open up chunk slot
  const int index = static_cast<int>(chunk - &m_ChunkArray[0]);
  EN_ASSERT(&m_ChunkArray[index] == chunk, "Calculated index does not correspond to given pointer!");
  m_OpenChunkSlots.push_back(index);

  // Delete chunk data
  m_ChunkArray[index].reset();
}

void ChunkManager::addToMap(Chunk* chunk, MapType mapType)
{
  const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

  int64_t key = createKey(chunk->getIndex());
  auto insertionResult = m_Chunks[mapTypeID].insert({ key, chunk });
  bool insertionSuccess = insertionResult.second;
  EN_ASSERT(insertionSuccess, "Chunk is already in map!");
}

void ChunkManager::moveToMap(Chunk* chunk, MapType source, MapType destination)
{
  const uint8_t sourceTypeID = static_cast<uint8_t>(source);

  int64_t key = createKey(chunk->getIndex());
  addToMap(chunk, destination);
  m_Chunks[sourceTypeID].erase(key);
}