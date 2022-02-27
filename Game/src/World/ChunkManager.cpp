#include "GMpch.h"
#include "ChunkManager.h"
#include "ChunkRenderer.h"
#include "Player/Player.h"
#include "Util/Noise.h"
#include <glm/gtc/matrix_access.hpp>

ChunkManager::ChunkManager()
{
  m_ChunkArray = new Chunk[s_MaxChunks];

  m_OpenChunkSlots.reserve(s_MaxChunks);
  for (int i = 0; i < s_MaxChunks; ++i)
    m_OpenChunkSlots.push_back(i);

  initializeLODs();
}

ChunkManager::~ChunkManager()
{
  delete[] m_ChunkArray;
  m_ChunkArray = nullptr;
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

    if (!isInRange(chunk->getGlobalIndex(), s_UnloadDistance))
      chunksToRemove.push_back(chunk);
  }
  for (int i = 0; i < chunksToRemove.size(); ++i)
    unloadChunk(chunksToRemove[i]);

  // Destroy heightmaps outside of unload range
  std::vector<int64_t> heightMapsToRemove{};
  for (auto& pair : m_HeightMaps)
  {
    const HeightMap& heightMap = pair.second;

    if (!isInRange(GlobalIndex({ heightMap.chunkI, heightMap.chunkJ, Player::OriginIndex().k }), s_UnloadDistance))
      heightMapsToRemove.push_back(createHeightMapKey(heightMap.chunkI, heightMap.chunkJ));
  }
  for (int i = 0; i < heightMapsToRemove.size(); ++i)
    m_HeightMaps.erase(heightMapsToRemove[i]);
}

void ChunkManager::render() const
{
  EN_PROFILE_FUNCTION();

  if (s_RenderDistance == 0)
    return;
  
  std::array<Vec4, 6> frustumPlanes = calculateViewFrustumPlanes(Player::Camera());

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
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

    if (isInRange(chunk->getGlobalIndex(), s_RenderDistance))
    {
      if (isInFrustum(chunk->center(), frustumPlanes))
        ChunkRenderer::DrawChunk(chunk);
    }
  }
  ChunkRenderer::EndScene();
}

bool ChunkManager::loadNewChunks(int maxNewChunks)
{
  EN_PROFILE_FUNCTION();

  // If there are no open chunk slots, don't load any more
  if (m_OpenChunkSlots.size() == 0)
    return false;

  // Load First chunk if none exist
  if (m_OpenChunkSlots.size() == s_MaxChunks)
    loadChunk(Player::OriginIndex());

  // Find new chunks to generate
  std::vector<GlobalIndex> newChunks = searchForChunks(maxNewChunks);

  // Load newly found chunks
  loadChunks(newChunks);

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

void ChunkManager::renderLODs()
{
  EN_PROFILE_FUNCTION();

  std::vector<LOD::Octree::Node*> leaves = m_LODTree.getLeaves();

  const std::array<Vec4, 6> frustumPlanes = calculateViewFrustumPlanes(Player::Camera());
  std::array<Vec4, 6> shiftedFrustumPlanes = frustumPlanes;
  std::array<length_t, 6> planeNormalMags = { glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Left)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Right)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Bottom)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Top)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Near)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Far)])), };

  ChunkRenderer::BeginScene(Player::Camera());
  for (auto it = leaves.begin(); it != leaves.end(); ++it)
  {
    LOD::Octree::Node* node = *it;

    if (node->data->vertexArray != nullptr)
    {
      // Shift each plane by distance equal to radius of sphere that circumscribes LOD
      static constexpr float sqrt3 = 1.732050807568877f;
      const length_t LODSphereRadius = sqrt3 * node->length() / 2;
      for (int planeID = 0; planeID < 6; ++planeID)
        shiftedFrustumPlanes[planeID].w = frustumPlanes[planeID].w + LODSphereRadius * planeNormalMags[planeID];

      if (isInFrustum(node->center(), shiftedFrustumPlanes) && !isInRange(node->anchor, s_RenderDistance - 1))
        ChunkRenderer::DrawLOD(node);
    }
  }
  ChunkRenderer::EndScene();
}

void ChunkManager::manageLODs()
{
  EN_PROFILE_FUNCTION();
  
  std::vector<LOD::Octree::Node*> leaves = m_LODTree.getLeaves();
  bool treeModified = splitAndCombineLODs(leaves);

  if (treeModified)
  {
    int totalVerts = 0;

    leaves = m_LODTree.getLeaves();
    for (auto it = leaves.begin(); it != leaves.end(); ++it)
    {
      LOD::Octree::Node* node = *it;

      if (!node->data->meshGenerated)
        LOD::GenerateMesh(node);

      LOD::UpdateMesh(m_LODTree, node);

      totalVerts += static_cast<int>(node->data->meshData.size());
      for (BlockFace face : BlockFaceIterator())
        totalVerts += static_cast<int>(node->data->transitionMeshData.size());
    }
    EN_INFO("Total LOD Vertices: {0}", totalVerts);
  }
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
    const int mapTypeID = static_cast<int>(mapType);

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

bool ChunkManager::isInRange(const GlobalIndex& chunkIndex, globalIndex_t range) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(chunkIndex[i] - Player::OriginIndex()[i]) > range)
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
  frustumPlanes[static_cast<int>(FrustumPlane::Left)] = row4 + row1;
  frustumPlanes[static_cast<int>(FrustumPlane::Right)] = row4 - row1;
  frustumPlanes[static_cast<int>(FrustumPlane::Bottom)] = row4 + row2;
  frustumPlanes[static_cast<int>(FrustumPlane::Top)] = row4 - row2;
  frustumPlanes[static_cast<int>(FrustumPlane::Near)] = row4 + row3;
  frustumPlanes[static_cast<int>(FrustumPlane::Far)] = row4 - row3;

  return frustumPlanes;
}

bool ChunkManager::isInFrustum(const Vec3& point, const std::array<Vec4, 6>& frustumPlanes) const
{
  for (int planeID = 0; planeID < 5; ++planeID) // Skip far plane
    if (glm::dot(Vec4(point, 1.0), frustumPlanes[planeID]) < 0)
      return false;

  return true;  
}

HeightMap ChunkManager::generateHeightMap(globalIndex_t chunkI, globalIndex_t chunkJ)
{
  HeightMap heightMap{};
  heightMap.chunkI = chunkI;
  heightMap.chunkJ = chunkJ;
  heightMap.maxHeight = -std::numeric_limits<length_t>::infinity();

  for (int i = 0; i < Chunk::Size(); ++i)
    for (int j = 0; j < Chunk::Size(); ++j)
    {
      Vec2 blockXY = Chunk::Length() * Vec2(chunkI, chunkJ) + Block::Length() * (Vec2(i, j) + Vec2(0.5));
      length_t terrainHeight = Noise::FastTerrainNoise2D(blockXY);
      heightMap.terrainHeights[i][j] = terrainHeight;

      if (terrainHeight > heightMap.maxHeight)
        heightMap.maxHeight = terrainHeight;
    }
  return heightMap;
}

void ChunkManager::setNeighbors(Chunk* chunk)
{
  const GlobalIndex& chunkIndex = chunk->getGlobalIndex();

  // Set neighbors in all directions
  for (BlockFace face : BlockFaceIterator())
  {
    // Store index of chunk adjacent to new chunk in direction of face
    const GlobalIndex neighborIndex = chunkIndex + GlobalIndex::OutwardNormal(face);

    // Find and add any existing neighbors to new chunk
    int64_t key = createKey(neighborIndex);
    for (MapType mapType : MapTypeIterator())
    {
      const int mapTypeID = static_cast<int>(mapType);

      if (m_Chunks[mapTypeID].find(key) != m_Chunks[mapTypeID].end())
      {
        Chunk* neighboringChunk = m_Chunks[mapTypeID][key];
        chunk->setNeighbor(face, neighboringChunk);
        neighboringChunk->setNeighbor(!face, chunk);

        // Renderable chunks should receive an update if they get a new neighbor
        if (mapType == MapType::Renderable)
          neighboringChunk->update();

        break;
      }
    }
  }
}

std::vector<GlobalIndex> ChunkManager::searchForChunks(int maxNewChunks)
{
  std::vector<GlobalIndex> newChunks{};
  for (auto& pair : m_BoundaryChunks)
  {
    Chunk* chunk = pair.second;

    for (BlockFace face : BlockFaceIterator())
      if (chunk->getNeighbor(face) == nullptr && !chunk->isFaceOpaque(face))
      {
        // Store index of potential new chunk
        const GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face);

        // If potential chunk is out of load range, skip it
        if (!isInRange(neighborIndex, s_LoadDistance))
          continue;

        newChunks.push_back(neighborIndex);
      }

    if (newChunks.size() >= maxNewChunks)
      break;
  }
  return newChunks;
}

Chunk* ChunkManager::loadChunk(const GlobalIndex& chunkIndex)
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

void ChunkManager::loadChunks(const std::vector<GlobalIndex>& newChunks)
{
  for (int i = 0; i < newChunks.size(); ++i)
  {
    const GlobalIndex newChunkIndex = newChunks[i];
    int64_t key = createKey(newChunkIndex);

    // Create key for hash map.  If chunk is already in a map, skip it
    bool isInMap = false;
    for (MapType mapType : MapTypeIterator())
    {
      const int mapTypeID = static_cast<int>(mapType);

      if (m_Chunks[mapTypeID].find(key) != m_Chunks[mapTypeID].end())
      {
        isInMap = true;
        break;
      }
    }
    if (isInMap)
      continue;

    // Generate Chunk
    Chunk* newChunk = loadChunk(newChunkIndex);
    setNeighbors(newChunk);

    // If there are no open chunk slots, don't load any more
    if (m_OpenChunkSlots.size() == 0)
      break;
  }
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
    EN_ASSERT(m_Chunks[static_cast<int>(source)].find(createKey(chunkNeighbor->getGlobalIndex())) != m_Chunks[static_cast<int>(source)].end(), "Chunk is not in correct map!");
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
    const int mapTypeID = static_cast<int>(mapType);

    auto pair = m_Chunks[mapTypeID].find(key);
    if (pair != m_Chunks[mapTypeID].end())
      return (*pair).second;
  }
  return nullptr;
}

void ChunkManager::addToMap(Chunk* const chunk, MapType mapType)
{
  const int mapTypeID = static_cast<int>(mapType);

  int64_t key = createKey(chunk->getGlobalIndex());
  auto insertionResult = m_Chunks[mapTypeID].insert({ key, chunk });
  bool insertionSuccess = insertionResult.second;
  EN_ASSERT(insertionSuccess, "Chunk is already in map!");
}

void ChunkManager::moveToMap(Chunk* const chunk, MapType source, MapType destination)
{
  const int sourceTypeID = static_cast<int>(source);

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

void ChunkManager::initializeLODs()
{
  std::vector<LOD::Octree::Node*> leaves{};

  bool treeModified = true;
  while (treeModified)
  {
    leaves = m_LODTree.getLeaves();
    treeModified = splitAndCombineLODs(leaves);
  }

  // Generate meshes for all LODs
  leaves = m_LODTree.getLeaves();
  for (auto it = leaves.begin(); it != leaves.end(); ++it)
  {
    LOD::Octree::Node* node = *it;
    LOD::GenerateMesh(node);
    LOD::UpdateMesh(m_LODTree, node);
  }
}

bool ChunkManager::splitLODs(std::vector<LOD::Octree::Node*>& leaves)
{
  bool treeModified = false;
  for (auto it = leaves.begin(); it != leaves.end();)
  {
    LOD::Octree::Node* node = *it;

    if (node->LODLevel() > 1)
    {
      int64_t splitRange = pow2(node->LODLevel() + 1) - 1 + s_RenderDistance;
      LOD::AABB splitRangeBoundingBox = { Player::OriginIndex() - splitRange, Player::OriginIndex() + splitRange };

      if (LOD::Intersection(splitRangeBoundingBox, node->boundingBox()))
      {
        m_LODTree.splitNode(node);
        treeModified = true;

        it = leaves.erase(it);
        continue;
      }
    }

    it++;
  }
  return treeModified;
}

bool ChunkManager::combineLODs(std::vector<LOD::Octree::Node*>& leaves)
{
  // Search for nodes to combine
  std::vector<LOD::Octree::Node*> cannibalNodes{};
  for (auto it = leaves.begin(); it != leaves.end(); ++it)
  {
    LOD::Octree::Node* node = *it;

    if (node->depth > 0)
    {
      int64_t combineRange = pow2(node->LODLevel() + 2) - 1 + s_RenderDistance;
      LOD::AABB rangeBoundingBox = { Player::OriginIndex() - combineRange, Player::OriginIndex() + combineRange };

      if (!LOD::Intersection(rangeBoundingBox, node->parent->boundingBox()))
        cannibalNodes.push_back(node->parent);
    }
  }

  // Combine nodes
  for (int i = 0; i < cannibalNodes.size(); ++i)
    m_LODTree.combineChildren(cannibalNodes[i]);

  return cannibalNodes.size() > 0;
}

bool ChunkManager::splitAndCombineLODs(std::vector<LOD::Octree::Node*>& leaves)
{
  bool nodesSplit = splitLODs(leaves);
  bool nodesCombined = combineLODs(leaves);
  return nodesSplit || nodesCombined;
}
