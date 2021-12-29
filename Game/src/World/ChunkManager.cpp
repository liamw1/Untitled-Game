#include "GMpch.h"
#include "ChunkManager.h"
#include "ChunkRenderer.h"
#include "Player/Player.h"
#include "Util/MarchingCubes.h"
#include "Util/Array2D.h"
#include <glm/gtc/matrix_access.hpp>

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

  static constexpr int8_t normals[6][3] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  // If there are no open chunk slots, don't load any more
  if (m_OpenChunkSlots.size() == 0)
    return false;

  // Load First chunk if none exist
  if (m_OpenChunkSlots.size() == s_MaxChunks)
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
          neighborIndex[i] += normals[static_cast<int>(face)][i];

        // If potential chunk is out of load range, skip it
        if (!isInRange(neighborIndex, s_LoadDistance))
          continue;

        newChunks.push_back(neighborIndex);
      }

    if (newChunks.size() >= maxNewChunks)
      break;
  }

  // Load new chunks
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
    Chunk* newChunk = loadNewChunk(newChunkIndex);

    // Set neighbors in all directions
    for (BlockFace dir : BlockFaceIterator())
    {
      // Store index of chunk adjacent to new chunk in direction "dir"
      GlobalIndex adjIndex = newChunkIndex;
      for (int i = 0; i < 3; ++i)
        adjIndex[i] += normals[static_cast<int>(dir)][i];

      // Find and add any existing neighbors to new chunk
      int64_t adjKey = createKey(adjIndex);
      for (MapType mapType : MapTypeIterator())
      {
        const int mapTypeID = static_cast<int>(mapType);

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

void ChunkManager::manageLODs()
{
  EN_PROFILE_FUNCTION();
  
  std::vector<LOD::Octree::Node*> leaves = m_LODTree.getLeaves();

  // Render LODs
  ChunkRenderer::BeginScene(Player::Camera());
  for (int n = 0; n < leaves.size(); ++n)
  {
    LOD::Octree::Node* node = leaves[n];
    if (node->data->vertexArray != nullptr)
      ChunkRenderer::DrawLOD(node);
  }
  ChunkRenderer::EndScene();

  // Split close nodes and load children
  for (auto it = leaves.begin(); it != leaves.end(); ++it)
  {
    LOD::Octree::Node* node = *it;
    int lodLevel = node->LODLevel();

    if (lodLevel > 0)
    {
      int64_t splitRange = pow2(lodLevel + 1) - 1 + s_RenderDistance;
      LOD::AABB splitRangeBoundingBox = { Player::OriginIndex() - splitRange, Player::OriginIndex() + splitRange };

      if (LOD::Intersection(splitRangeBoundingBox, node->boundingBox()))
      {
        m_LODTree.splitNode(node);

          // Load children
        loadNewLODs(node, false);

        it = leaves.erase(it);
        it--;
      }
    }
  }

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
  {
    m_LODTree.combineChildren(cannibalNodes[i]);
    loadNewLODs(cannibalNodes[i], true);
  }

  EN_INFO("{0}", leaves.size());
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
      length_t terrainHeight = 150 * Block::Length() * glm::simplex(blockXY / 1280.0 / Block::Length()) + 50 * Block::Length() * glm::simplex(blockXY / 320.0 / Block::Length()) + 5 * Block::Length() * glm::simplex(blockXY / 40.0 / Block::Length());
      heightMap.terrainHeights[i][j] = terrainHeight;

      if (terrainHeight > heightMap.maxHeight)
        heightMap.maxHeight = terrainHeight;
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

void ChunkManager::loadNewLODs(LOD::Octree::Node* node, bool combineMode)
{
  EN_ASSERT(node->LODLevel() > 0, "LOD level cannot be 0!");

  struct Vertex
  {
    Vec3 position;
    int quadIndex;
    float lightValue;
  };

  const int subDivisions = combineMode ? Chunk::Size() : 2 * Chunk::Size();

  const length_t lodLength = pow2(node->LODLevel()) * Chunk::Length();
  const length_t subDivisionLength = lodLength / subDivisions;

  // Generate heightmap
  length_t minHeight = std::numeric_limits<length_t>::max();
  length_t maxHeight = -std::numeric_limits<length_t>::max();
  Array2D<length_t> terrainHeights = Array2D<length_t>(subDivisions + 1, subDivisions + 1);
  for (int i = 0; i < subDivisions + 1; ++i)
    for (int j = 0; j < subDivisions + 1; ++j)
    {
      Vec2 pointXY = Chunk::Length() * static_cast<Vec2>(node->anchor) + subDivisionLength * Vec2(i, j);
      length_t terrainHeight = 150 * Block::Length() * glm::simplex(pointXY / 1280.0 / Block::Length()) + 50 * Block::Length() * glm::simplex(pointXY / 320.0 / Block::Length()) + 5 * Block::Length() * glm::simplex(pointXY / 40.0 / Block::Length());
      terrainHeights[i][j] = terrainHeight;

      if (terrainHeight > maxHeight)
        maxHeight = terrainHeight;
      if (terrainHeight < minHeight)
        minHeight = terrainHeight;
    }

  if (node->anchor.k * Chunk::Length() > maxHeight || node->anchor.k * Chunk::Length() + lodLength < minHeight)
    return;

  // Generate meshes
  const int N = combineMode ? 1 : 8;
  for (int n = 0; n < N; ++n)
  {
    int i0 = n & bit(2) ? Chunk::Size() : 0;
    int j0 = n & bit(1) ? Chunk::Size() : 0;
    int k0 = n & bit(0) ? Chunk::Size() : 0;

    std::vector<Vertex> LODMesh{};
    for (int i = i0; i < i0 + Chunk::Size(); ++i)
      for (int j = j0; j < j0 + Chunk::Size(); ++j)
        for (int k = k0; k < k0 + Chunk::Size(); ++k)
        {
          const length_t subDivisionHeight = node->anchor.k * Chunk::Length() + k * subDivisionLength;

          uint8_t cubeIndex = 0;
          if (terrainHeights[i][j] > subDivisionHeight)
            cubeIndex |= bit(3);
          if (terrainHeights[i][static_cast<int64_t>(j) + 1] > subDivisionHeight)
            cubeIndex |= bit(0);
          if (terrainHeights[static_cast<int64_t>(i) + 1][j] > subDivisionHeight)
            cubeIndex |= bit(2);
          if (terrainHeights[static_cast<int64_t>(i) + 1][static_cast<int64_t>(j) + 1] > subDivisionHeight)
            cubeIndex |= bit(1);
          if (terrainHeights[i][j] > subDivisionHeight + subDivisionLength)
            cubeIndex |= bit(7);
          if (terrainHeights[i][static_cast<int64_t>(j) + 1] > subDivisionHeight + subDivisionLength)
            cubeIndex |= bit(4);
          if (terrainHeights[static_cast<int64_t>(i) + 1][j] > subDivisionHeight + subDivisionLength)
            cubeIndex |= bit(6);
          if (terrainHeights[static_cast<int64_t>(i) + 1][static_cast<int64_t>(j) + 1] > subDivisionHeight + subDivisionLength)
            cubeIndex |= bit(5);

          for (int tri = 0; tri < 5; ++tri)
          {
            const std::array<int8_t, 3> edgeIndices = a2iTriangleConnectionTable[cubeIndex][tri];

            if (edgeIndices[0] < 0)
              break;

            // Local position of vertex within LOD
            std::array<Vec3, 3> vertexPositions{};
            for (int v = 0; v < 3; ++v)
              vertexPositions[v] = subDivisionLength * Vec3(i - i0, j - j0, k - k0) + subDivisionLength * edgeOffsets[edgeIndices[v]];

            // Calculate vertex normal
            Vec3 normal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));
            float lightValue = static_cast<float>((2.0 + normal.z) / 3);

            for (int v = 0; v < 3; ++v)
              LODMesh.push_back({ vertexPositions[v], v, lightValue });
          }
        }

    // Generate vertex array
    Engine::Shared<Engine::VertexArray> LODVertexArray = Engine::VertexArray::Create();
    auto LODVertexBuffer = Engine::VertexBuffer::Create(static_cast<uint32_t>(LODMesh.size()) * sizeof(Vertex));
    LODVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                 { ShaderDataType::Int,    "a_QuadIndex"},
                                 { ShaderDataType::Float,  "s_LightValue"} });
    LODVertexArray->addVertexBuffer(LODVertexBuffer);

    uintptr_t dataSize = sizeof(Vertex) * LODMesh.size();
    LODVertexBuffer->setData(LODMesh.data(), dataSize);

    LOD::Octree::Node* meshNode = combineMode ? node : node->children[n];
    meshNode->data->vertexArray = LODVertexArray;
    meshNode->data->meshSize = static_cast<uint32_t>(LODMesh.size());
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
