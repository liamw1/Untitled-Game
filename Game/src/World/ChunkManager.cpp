#include "GMpch.h"
#include "ChunkManager.h"
#include "Player/Player.h"
#include "Util/Noise.h"

ChunkManager::ChunkManager()
{
  m_ChunkArray = new Chunk[s_MaxChunks];

  m_OpenChunkSlots.reserve(s_MaxChunks);
  for (int i = 0; i < s_MaxChunks; ++i)
    m_OpenChunkSlots.push_back(i);
}

ChunkManager::~ChunkManager()
{
  delete[] m_ChunkArray;
  m_ChunkArray = nullptr;
}

void ChunkManager::initialize()
{
  while (loadNewChunks(10000))
  {
  }

  initializeLODs();
}

void ChunkManager::clean()
{
  EN_PROFILE_FUNCTION();

  // If no boundary chunks exist, move chunks to boundary chunk map
  if (m_BoundaryChunks.size() == 0)
    for (auto& [key, chunk] : m_RenderableChunks)
      moveToMap(chunk, MapType::Renderable, MapType::Boundary);

  // Destroy boundary chunks outside of unload range
  std::vector<Chunk*> chunksToRemove{};
  for (auto& [key, chunk] : m_BoundaryChunks)
    if (!isInRange(chunk->getGlobalIndex(), s_UnloadDistance))
      chunksToRemove.push_back(chunk);
  for (int i = 0; i < chunksToRemove.size(); ++i)
    unloadChunk(chunksToRemove[i]);

  // Destroy heightmaps outside of unload range
  std::vector<int> heightMapsToRemove{};
  for (auto& [key, heightMap] : m_HeightMaps)
    if (!isInRange(GlobalIndex(heightMap.chunkI, heightMap.chunkJ, Player::OriginIndex().k), s_UnloadDistance))
      heightMapsToRemove.push_back(createHeightMapKey(heightMap.chunkI, heightMap.chunkJ));
  for (int i = 0; i < heightMapsToRemove.size(); ++i)
    m_HeightMaps.erase(heightMapsToRemove[i]);
}

void ChunkManager::render() const
{
  EN_PROFILE_FUNCTION();

  if (s_RenderDistance == 0)
    return;

  const Mat4& viewProjection = Engine::Scene::ActiveCameraViewProjection();
  
  std::array<Vec4, 6> frustumPlanes = calculateViewFrustumPlanes(viewProjection);

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
  static constexpr float sqrt3 = 1.732050807568877f;
  static constexpr length_t chunkSphereRadius = sqrt3 * Chunk::Length() / 2;
  for (int planeID = 0; planeID < 6; ++planeID)
  {
    const length_t planeNormalMag = glm::length(Vec3(frustumPlanes[planeID]));
    frustumPlanes[planeID].w += chunkSphereRadius * planeNormalMag;
  }

  // Render chunks in view frustum
  Chunk::BindBuffers();
  for (auto& [key, chunk] : m_RenderableChunks)
    if (isInRange(chunk->getGlobalIndex(), s_RenderDistance))
      if (isInFrustum(chunk->center(), frustumPlanes))
        chunk->draw();
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
  std::vector<GlobalIndex> newChunks{};
  for (auto& [key, chunk] : m_BoundaryChunks)
  {
    for (Block::Face face : Block::FaceIterator())
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

  // Load newly found chunks
  for (int i = 0; i < newChunks.size(); ++i)
  {
    const GlobalIndex newChunkIndex = newChunks[i];
    int key = createKey(newChunkIndex);

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

    // If there are no open chunk slots, don't load any more
    if (m_OpenChunkSlots.size() == 0)
      break;
  }

  // Move chunk pointers out of m_BoundaryChunks when all their neighbors are accounted for
  for (auto& [key, chunk] : m_BoundaryChunks)
    if (!isOnBoundary(chunk))
    {
      MapType destination = chunk->isEmpty() ? MapType::Empty : MapType::Renderable;
      moveToMap(chunk, MapType::Boundary, destination);

      chunk->update();
    }

  return newChunks.size() > 0;
}

void ChunkManager::renderLODs()
{
  EN_PROFILE_FUNCTION();

  std::vector<LOD::Octree::Node*> leaves = m_LODTree.getLeaves();
  const Mat4& viewProjection = Engine::Scene::ActiveCameraViewProjection();

  const std::array<Vec4, 6> frustumPlanes = calculateViewFrustumPlanes(viewProjection);
  std::array<Vec4, 6> shiftedFrustumPlanes = frustumPlanes;
  std::array<length_t, 6> planeNormalMags = { glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Left)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Right)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Bottom)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Top)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Near)])),
                                              glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Far)])), };

  LOD::BindBuffers();
  for (LOD::Octree::Node* leaf : leaves)
    if (leaf->data->primaryMesh.vertexArray != nullptr)
    {
      // Shift each plane by distance equal to radius of sphere that circumscribes LOD
      static constexpr float sqrt3 = 1.732050807568877f;
      const length_t LODSphereRadius = sqrt3 * leaf->length() / 2;
      for (int planeID = 0; planeID < 6; ++planeID)
        shiftedFrustumPlanes[planeID].w = frustumPlanes[planeID].w + LODSphereRadius * planeNormalMags[planeID];

      if (isInFrustum(leaf->center(), shiftedFrustumPlanes) && !isInRange(leaf->anchor, s_RenderDistance - 1))
        LOD::Draw(leaf);
    }
}

void ChunkManager::manageLODs()
{
  EN_PROFILE_FUNCTION();
  
  std::vector<LOD::Octree::Node*> leaves = m_LODTree.getLeaves();
  bool treeModified = splitAndCombineLODs(leaves);

  if (treeModified)
  {
    std::vector<LOD::Octree::Node*> leaves = m_LODTree.getLeaves();
    for (LOD::Octree::Node* leaf : leaves)
    {
      if (!leaf->data->meshGenerated)
        LOD::GenerateMesh(leaf);

      if (leaf->data->needsUpdate && leaf->data->primaryMesh.vertices.size() > 0)
        LOD::UpdateMesh(m_LODTree, leaf);
    }
  }
}

Chunk* ChunkManager::findChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return findChunk(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void ChunkManager::sendChunkUpdate(Chunk* const chunk)
{
  if (chunk == nullptr)
    return;

  int key = createKey(chunk->getGlobalIndex());

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

int ChunkManager::createKey(const GlobalIndex& chunkIndex) const
{
  return chunkIndex.i % bitUi32(10) + bitUi32(10) * (chunkIndex.j % bitUi32(10)) + bitUi32(20) * (chunkIndex.k % bitUi32(10));
}

int ChunkManager::createHeightMapKey(globalIndex_t chunkI, globalIndex_t chunkJ) const
{
  return createKey(GlobalIndex(chunkI, chunkJ, 0));
}

bool ChunkManager::isInRange(const GlobalIndex& chunkIndex, globalIndex_t range) const
{
  for (int i = 0; i < 3; ++i)
    if (abs(chunkIndex[i] - Player::OriginIndex()[i]) > range)
      return false;
  return true;
}

std::array<Vec4, 6> ChunkManager::calculateViewFrustumPlanes(const Mat4& viewProjection) const
{
  Vec4 row1 = glm::row(viewProjection, 0);
  Vec4 row2 = glm::row(viewProjection, 1);
  Vec4 row3 = glm::row(viewProjection, 2);
  Vec4 row4 = glm::row(viewProjection, 3);

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
      heightMap.surfaceData[i][j] = Noise::FastTerrainNoise2D(blockXY);

      if (heightMap.surfaceData[i][j].getHeight() > heightMap.maxHeight)
        heightMap.maxHeight = heightMap.surfaceData[i][j].getHeight();
    }
  return heightMap;
}

Chunk* ChunkManager::loadChunk(const GlobalIndex& chunkIndex)
{
  EN_ASSERT(m_OpenChunkSlots.size() > 0, "All chunks slots are full!");

  // Grab first open chunk slot
  int chunkSlot = m_OpenChunkSlots.back();
  m_OpenChunkSlots.pop_back();

  // Generate heightmap is none exists
  int heightMapKey = createHeightMapKey(chunkIndex.i, chunkIndex.j);
  if (m_HeightMaps.find(heightMapKey) == m_HeightMaps.end())
    m_HeightMaps.insert({ heightMapKey, generateHeightMap(chunkIndex.i, chunkIndex.j) });

  // Insert chunk into array and load it
  m_ChunkArray[chunkSlot] = std::move(Chunk(chunkIndex));
  Chunk* newChunk = &m_ChunkArray[chunkSlot];
  newChunk->load(m_HeightMaps[heightMapKey]);

  // Insert chunk pointer into boundary chunk map
  addToMap(newChunk, MapType::Boundary);

  // Set neighbors in all directions
  for (Block::Face face : Block::FaceIterator())
  {
    const GlobalIndex neighborIndex = chunkIndex + GlobalIndex::OutwardNormal(face);

    // Find and add neighbor to new chunk, if it exists
    int key = createKey(neighborIndex);
    for (MapType mapType : MapTypeIterator())
    {
      const int mapTypeID = static_cast<int>(mapType);

      if (m_Chunks[mapTypeID].find(key) != m_Chunks[mapTypeID].end())
      {
        Chunk* neighboringChunk = m_Chunks[mapTypeID][key];
        newChunk->setNeighbor(face, neighboringChunk);
        neighboringChunk->setNeighbor(!face, newChunk);

        // Renderable chunks should receive an update if they get a new neighbor
        if (mapType == MapType::Renderable)
          neighboringChunk->update();

        break;
      }
    }
  }

  return newChunk;
}

void ChunkManager::unloadChunk(Chunk* const chunk)
{
  EN_ASSERT(m_BoundaryChunks.find(createKey(chunk->getGlobalIndex())) != m_BoundaryChunks.end(), "Chunk is not in boundary chunk map!");

  // Move neighbors to m_BoundaryChunks
  for (Block::Face face : Block::FaceIterator())
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
  int key = createKey(chunk->getGlobalIndex());
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
  const int key = createKey(globalIndex);

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

  int key = createKey(chunk->getGlobalIndex());
  auto insertionResult = m_Chunks[mapTypeID].insert({ key, chunk });
  bool insertionSuccess = insertionResult.second;
  EN_ASSERT(insertionSuccess, "Chunk is already in map!");
}

void ChunkManager::moveToMap(Chunk* const chunk, MapType source, MapType destination)
{
  const int sourceTypeID = static_cast<int>(source);

  int key = createKey(chunk->getGlobalIndex());
  addToMap(chunk, destination);
  m_Chunks[sourceTypeID].erase(key);
}

bool ChunkManager::isOnBoundary(const Chunk* const chunk) const
{
  for (Block::Face face : Block::FaceIterator())
    if (chunk->getNeighbor(face) == nullptr && !chunk->isFaceOpaque(face))
      return true;
  return false;
}

void ChunkManager::initializeLODs()
{
  EN_PROFILE_FUNCTION();

  std::vector<LOD::Octree::Node*> leaves{};

  bool treeModified = true;
  while (treeModified)
  {
    leaves = m_LODTree.getLeaves();
    treeModified = splitAndCombineLODs(leaves);
  }

  // Generate meshes for all LODs
  leaves = m_LODTree.getLeaves();
  for (LOD::Octree::Node* leaf : leaves)
  {
    LOD::GenerateMesh(leaf);

    if (leaf->data->primaryMesh.vertices.size() > 0)
      LOD::UpdateMesh(m_LODTree, leaf);
  }
}

bool ChunkManager::splitLODs(std::vector<LOD::Octree::Node*>& leaves)
{
  bool treeModified = false;
  for (auto it = leaves.begin(); it != leaves.end();)
  {
    LOD::Octree::Node* node = *it;

    if (node->LODLevel() > 0)
    {
      globalIndex_t splitRange = 2 * node->size() - 1 + s_RenderDistance;
      LOD::AABB splitRangeBoundingBox = { Player::OriginIndex() - splitRange, Player::OriginIndex() + splitRange };

      if (LOD::Intersection(splitRangeBoundingBox, node->boundingBox()))
      {
        m_LODTree.splitNode(node);
        LOD::MessageNeighbors(m_LODTree, node);

        it = leaves.erase(it);
        treeModified = true;
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
  for (LOD::Octree::Node* node : leaves)
    if (node->depth > 0)
    {
      globalIndex_t combineRange = 4 * node->size() - 1 + s_RenderDistance;
      LOD::AABB rangeBoundingBox = { Player::OriginIndex() - combineRange, Player::OriginIndex() + combineRange };

      if (!LOD::Intersection(rangeBoundingBox, node->parent->boundingBox()))
        cannibalNodes.push_back(node->parent);
    }

  // Combine nodes
  for (LOD::Octree::Node* node : cannibalNodes)
  {
    m_LODTree.combineChildren(node);
    LOD::MessageNeighbors(m_LODTree, node);
  }

  return cannibalNodes.size() > 0;
}

bool ChunkManager::splitAndCombineLODs(std::vector<LOD::Octree::Node*>& leaves)
{
  bool nodesSplit = splitLODs(leaves);
  bool nodesCombined = combineLODs(leaves);
  return nodesSplit || nodesCombined;
}
