#include "GMpch.h"
#include "ChunkManager.h"
#include "ChunkRenderer.h"
#include "Player/Player.h"
#include <glm/gtc/matrix_access.hpp>

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

  // If no boundary chunks exist, move chunks to boundary chunk map
  if (m_BoundaryChunks.size() == 0)
    for (auto& pair : m_RenderableChunks)
    {
      Chunk* chunk = pair.second;
      moveToMap(chunk, MapType::Renderable, MapType::Boundary);
    }

  // Destroy boundary chunks outside of unload range
  std::vector<Chunk*> chunksToRemove{};
  for (auto& pair : m_BoundaryChunks)
  {
    Chunk* chunk = pair.second;

    if (isOutOfRange(chunk->getLocalIndex()))
      chunksToRemove.push_back(chunk);
  }
  for (int i = 0; i < chunksToRemove.size(); ++i)
    unloadChunk(chunksToRemove[i]);

  // Destroy heightmaps outside of unload range
  std::vector<int64_t> heightMapsToRemove{};
  for (auto& pair : m_HeightMaps)
  {
    const HeightMap& heightMap = pair.second;

    if (isOutOfRange(GlobalIndex({ heightMap.chunkI, heightMap.chunkJ, Player::OriginIndex().k })))
      heightMapsToRemove.push_back(createHeightMapKey(heightMap.chunkI, heightMap.chunkJ));
  }
  for (int i = 0; i < heightMapsToRemove.size(); ++i)
    m_HeightMaps.erase(heightMapsToRemove[i]);
}

void ChunkManager::render() const
{
  EN_PROFILE_FUNCTION();
  
  std::array<Vec4, 6> frustumPlanes = calculateViewFrustumPlanes(Player::Camera());

  // Shift each plane by an amount equal to radius of a sphere that contains chunk
  static constexpr float sqrt3 = 1.732050807568877f;
  static constexpr length_t chunkSphereRadius = sqrt3 * Chunk::Length() / 2;
  for (int planeID = 0; planeID < 6; ++planeID)
  {
    const length_t planeNormalMag = glm::length(Vec3(frustumPlanes[planeID]));
    frustumPlanes[planeID].w += chunkSphereRadius * planeNormalMag;
  }

  // Render chunks in view frustum
  ChunkRenderer::BeginScene(Player::Camera());
  for (auto& pair : m_RenderableChunks)
  {
    Chunk* chunk = pair.second;

    if (isInRenderRange(chunk->getLocalIndex()))
    {
      if (isInFrustum(chunk->center(), frustumPlanes))
        ChunkRenderer::DrawChunk(chunk);
    }
  }
  ChunkRenderer::EndScene();
}

bool ChunkManager::loadNewChunks(uint32_t maxNewChunks)
{
  EN_PROFILE_FUNCTION();

  static constexpr int8_t normals[6][3] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  // If there are no open chunk slots, don't load any more
  if (m_OpenChunkSlots.size() == 0)
    return false;

  // Load First chunk if none exist
  if (m_OpenChunkSlots.size() == s_TotalPossibleChunks)
    loadNewChunk(Player::OriginIndex());

  // Find new chunks to generate
  std::vector<GlobalIndex> newChunks{};
  for (auto& pair : m_BoundaryChunks)
  {
    Chunk* chunk = pair.second;

    for (BlockFace face : BlockFaceIterator())
      if (chunk->getNeighbor(face) == nullptr && !chunk->isFaceOpaque(face))
      {
        // Store index of potential new chunk
        GlobalIndex neighborIndex = chunk->getGlobalIndex();
        for (int i = 0; i < 3; ++i)
          neighborIndex[i] += normals[static_cast<uint8_t>(face)][i];

        // If potential chunk is out of load range, skip it
        if (!isInLoadRange(neighborIndex))
          continue;

        newChunks.push_back(neighborIndex);
      }

    if (newChunks.size() >= maxNewChunks)
      break;
  }

  // Added new chunks to m_BoundaryChunks
  for (int i = 0; i < newChunks.size(); ++i)
  {
    const GlobalIndex newChunkIndex = newChunks[i];
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
      GlobalIndex adjIndex = newChunkIndex;
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

          // Renderable chunks should receive an update if they get a new neighbor
          if (mapType == MapType::Renderable)
            adjChunk->update();

          break;
        }
      }
    }

    // If there are no open chunk slots, don't load any more
    if (m_OpenChunkSlots.size() == 0)
      break;
  }

  // Move chunk pointers out of m_BoundaryChunks when all their neighbors are accounted for
  for (auto& pair : m_BoundaryChunks)
  {
    Chunk* chunk = pair.second;

    if (!isOnBoundary(chunk))
    {
      MapType destination = chunk->isEmpty() ? MapType::Empty : MapType::Renderable;
      moveToMap(chunk, MapType::Boundary, destination);

      chunk->update();
    }
  }

  return newChunks.size() > 0;
}

Chunk* ChunkManager::findChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return findChunk(GlobalIndex({ originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k }));
}

void ChunkManager::sendChunkUpdate(Chunk* const chunk)
{
  if (chunk == nullptr)
    return;

  int64_t key = createKey(chunk->getGlobalIndex());

  // Find maptype of chunk
  bool foundChunk = false;
  MapType source = MapType::First;
  for (MapType mapType : MapTypeIterator())
  {
    const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

    auto pair = m_Chunks[mapTypeID].find(key);
    if (pair != m_Chunks[mapTypeID].end())
    {
      foundChunk = true;
      source = mapType;
    }
  }

  if (!foundChunk)
    EN_ERROR("Chunk could not be found!");
  else
  {
    chunk->update();

    bool boundaryChunk = isOnBoundary(chunk);

    // Recategorize chunk if necessary.
    if (boundaryChunk && source != MapType::Boundary)
      moveToMap(chunk, source, MapType::Boundary);
    else if (!boundaryChunk && source == MapType::Boundary)
      moveToMap(chunk, MapType::Boundary, chunk->isEmpty() ? MapType::Empty : MapType::Renderable);
    else if (source == MapType::Renderable && chunk->isEmpty())
      moveToMap(chunk, MapType::Renderable, MapType::Empty);
    else if (source == MapType::Empty && !chunk->isEmpty())
      moveToMap(chunk, MapType::Empty, MapType::Renderable);
  }
}

int64_t ChunkManager::createKey(const GlobalIndex& chunkIndex) const
{
  return chunkIndex.i % bit(10) + bit(10) * (chunkIndex.j % bit(10)) + bit(20) * (chunkIndex.k % bit(10));
}

int64_t ChunkManager::createHeightMapKey(globalIndex_t chunkI, globalIndex_t chunkJ) const
{
  return chunkI % bit(10) + bit(10) * (chunkJ % bit(10));
}

bool ChunkManager::isOutOfRange(const LocalIndex& chunkIndex) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(chunkIndex[i]) > s_UnloadDistance)
      return true;
  return false;
}

bool ChunkManager::isOutOfRange(const GlobalIndex& chunkIndex) const
{
  LocalIndex relativeIndex = Chunk::CalcRelativeIndex(chunkIndex, Player::OriginIndex());

  for (int i = 0; i < 3; ++i)
    if (abs(relativeIndex[i]) > s_UnloadDistance)
      return true;
  return false;
}

bool ChunkManager::isInLoadRange(const LocalIndex& chunkIndex) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(chunkIndex[i]) > s_LoadDistance)
      return false;
  return true;
}

bool ChunkManager::isInLoadRange(const GlobalIndex& chunkIndex) const
{
  LocalIndex relativeIndex = Chunk::CalcRelativeIndex(chunkIndex, Player::OriginIndex());

  for (int i = 0; i < 3; ++i)
    if (abs(relativeIndex[i]) > s_LoadDistance)
      return false;
  return true;
}

bool ChunkManager::isInRenderRange(const LocalIndex& chunkIndex) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(chunkIndex[i]) > s_RenderDistance)
      return false;
  return true;
}

std::array<Vec4, 6> ChunkManager::calculateViewFrustumPlanes(const Engine::Camera& playerCamera) const
{
  const Mat4& viewProj = playerCamera.getViewProjectionMatrix();
  const Vec4 row1 = glm::row(viewProj, 0);
  const Vec4 row2 = glm::row(viewProj, 1);
  const Vec4 row3 = glm::row(viewProj, 2);
  const Vec4 row4 = glm::row(viewProj, 3);

  std::array<Vec4, 6> frustumPlanes{};
  frustumPlanes[static_cast<uint8_t>(FrustumPlane::Left)] = row4 + row1;
  frustumPlanes[static_cast<uint8_t>(FrustumPlane::Right)] = row4 - row1;
  frustumPlanes[static_cast<uint8_t>(FrustumPlane::Bottom)] = row4 + row2;
  frustumPlanes[static_cast<uint8_t>(FrustumPlane::Top)] = row4 - row2;
  frustumPlanes[static_cast<uint8_t>(FrustumPlane::Near)] = row4 + row3;
  frustumPlanes[static_cast<uint8_t>(FrustumPlane::Far)] = row4 - row3;

  return frustumPlanes;
}

bool ChunkManager::isInFrustum(const Vec3& point, const std::array<Vec4, 6>& frustumPlanes) const
{
  for (uint8_t planeID = 0; planeID < 5; ++planeID) // Skip far plane
    if (glm::dot(Vec4(point, 1.0), frustumPlanes[planeID]) < 0)
      return false;

  return true;  
}

HeightMap ChunkManager::generateHeightMap(globalIndex_t chunkI, globalIndex_t chunkJ)
{
  HeightMap heightMap{};
  heightMap.chunkI = chunkI;
  heightMap.chunkJ = chunkJ;

  for (uint8_t i = 0; i < Chunk::Size(); ++i)
    for (uint8_t j = 0; j < Chunk::Size(); ++j)
    {
      Vec2 blockXY = Vec2(chunkI * Chunk::Length() + i * Block::Length(), chunkJ * Chunk::Length() + j * Block::Length());
      length_t terrainHeight = 150 * Block::Length() * glm::simplex(blockXY / 1280.0 / Block::Length()) + 50 * Block::Length() * glm::simplex(blockXY / 320.0 / Block::Length()) + 5 * Block::Length() * glm::simplex(blockXY / 40.0 / Block::Length());
      heightMap.terrainHeights[i][j] = terrainHeight;
    }
  return heightMap;
}

Chunk* ChunkManager::loadNewChunk(const GlobalIndex& chunkIndex)
{
  EN_ASSERT(m_OpenChunkSlots.size() > 0, "All chunks slots are full!");

  // Grab first open chunk slot
  int chunkSlot = m_OpenChunkSlots.back();
  m_OpenChunkSlots.pop_back();

  // Generate heightmap is none exists
  int64_t heightMapKey = createHeightMapKey(chunkIndex.i, chunkIndex.j);
  if (m_HeightMaps.find(heightMapKey) == m_HeightMaps.end())
    m_HeightMaps[heightMapKey] = generateHeightMap(chunkIndex.i, chunkIndex.j);

  // Insert chunk into array and load it
  m_ChunkArray[chunkSlot] = std::move(Chunk(chunkIndex));
  Chunk* newChunk = &m_ChunkArray[chunkSlot];
  newChunk->load(m_HeightMaps[heightMapKey]);

  // Insert chunk pointer into boundary chunk map
  addToMap(newChunk, MapType::Boundary);

  return newChunk;
}

void ChunkManager::unloadChunk(Chunk* const chunk)
{
  EN_ASSERT(m_BoundaryChunks.find(createKey(chunk->getGlobalIndex())) != m_BoundaryChunks.end(), "Chunk is not in boundary chunk map!");

  // Move neighbors to m_BoundaryChunks
  for (BlockFace face : BlockFaceIterator())
  {
    Chunk* chunkNeighbor = chunk->getNeighbor(face);

    // If neighbor does not exist, skip
    if (chunkNeighbor == nullptr)
      continue;

    // Check if chunk is in m_BoundaryChunk.  If so, skip it
    if (m_BoundaryChunks.find(createKey(chunkNeighbor->getGlobalIndex())) != m_BoundaryChunks.end())
      continue;

    // Move chunk pointer to m_BoundaryChunks
    MapType source = chunkNeighbor->isEmpty() ? MapType::Empty : MapType::Renderable;
    EN_ASSERT(m_Chunks[static_cast<uint8_t>(source)].find(createKey(chunkNeighbor->getGlobalIndex())) != m_Chunks[static_cast<uint8_t>(source)].end(), "Chunk is not in correct map!");
    moveToMap(chunkNeighbor, source, MapType::Boundary);
  }

  // Remove chunk pointer from m_BoundaryChunks
  int64_t key = createKey(chunk->getGlobalIndex());
  bool eraseSuccessful = m_BoundaryChunks.erase(key);
  EN_ASSERT(eraseSuccessful, "Chunk is not in map!");

  // Open up chunk slot
  const int index = static_cast<int>(chunk - &m_ChunkArray[0]);
  EN_ASSERT(&m_ChunkArray[index] == chunk, "Calculated index does not correspond to given pointer!");
  m_OpenChunkSlots.push_back(index);

  // Delete chunk data
  m_ChunkArray[index].reset();
}

Chunk* ChunkManager::findChunk(const GlobalIndex& globalIndex) const
{
  const int64_t key = createKey(globalIndex);

  for (MapType mapType : MapTypeIterator())
  {
    const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

    auto pair = m_Chunks[mapTypeID].find(key);
    if (pair != m_Chunks[mapTypeID].end())
      return (*pair).second;
  }
  return nullptr;
}

void ChunkManager::addToMap(Chunk* const chunk, MapType mapType)
{
  const uint8_t mapTypeID = static_cast<uint8_t>(mapType);

  int64_t key = createKey(chunk->getGlobalIndex());
  auto insertionResult = m_Chunks[mapTypeID].insert({ key, chunk });
  bool insertionSuccess = insertionResult.second;
  EN_ASSERT(insertionSuccess, "Chunk is already in map!");
}

void ChunkManager::moveToMap(Chunk* const chunk, MapType source, MapType destination)
{
  const uint8_t sourceTypeID = static_cast<uint8_t>(source);

  int64_t key = createKey(chunk->getGlobalIndex());
  addToMap(chunk, destination);
  m_Chunks[sourceTypeID].erase(key);
}

bool ChunkManager::isOnBoundary(const Chunk* const chunk) const
{
  for (BlockFace face : BlockFaceIterator())
    if (chunk->getNeighbor(face) == nullptr && !chunk->isFaceOpaque(face))
      return true;
  return false;
}
