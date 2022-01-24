#include "GMpch.h"
#include "ChunkManager.h"
#include "ChunkRenderer.h"
#include "Player/Player.h"
#include "Util/TransVoxel.h"
#include "Util/Noise.h"
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
  {
    EN_PROFILE_SCOPE("LOD Rendering");

    std::array<Vec4, 6> frustumPlanes = calculateViewFrustumPlanes(Player::Camera());
    std::array<Vec4, 6> shiftedFrustumPlanes = frustumPlanes;
    std::array<length_t, 6> planeNormalMags = { glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Left)])),
                                                glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Right)])),
                                                glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Bottom)])),
                                                glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Top)])),
                                                glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Near)])),
                                                glm::length(Vec3(frustumPlanes[static_cast<int>(FrustumPlane::Far)])), };

    ChunkRenderer::BeginScene(Player::Camera());
    for (int n = 0; n < leaves.size(); ++n)
    {
      LOD::Octree::Node* node = leaves[n];

      if (node->data->vertexArray != nullptr && !isInRange(node->anchor, s_RenderDistance))
      {
        // Shift each plane by distance equal to radius of sphere that circumscribes LOD
        static constexpr float sqrt3 = 1.732050807568877f;
        const length_t LODSphereRadius = sqrt3 * node->length() / 2;
        for (int planeID = 0; planeID < 6; ++planeID)
          shiftedFrustumPlanes[planeID].w = frustumPlanes[planeID].w + LODSphereRadius * planeNormalMags[planeID];

        if (isInFrustum(node->center(), shiftedFrustumPlanes))
          ChunkRenderer::DrawLOD(node);
      }
    }
    ChunkRenderer::EndScene();
  }

  // Split close nodes and load children
  for (auto it = leaves.begin(); it != leaves.end();)
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

        for (int i = 0; i < 8; ++i)
          generateLODMesh(node->children[i]);

        it = leaves.erase(it);
        continue;
      }
    }

    it++;
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
    generateLODMesh(cannibalNodes[i]);
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
      length_t terrainHeight = terrainFunction(blockXY);
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

void ChunkManager::generateLODMesh(LOD::Octree::Node* node)
{
  // LOD smoothness parameter, must be in the range [0.0, 1.0]
#if 1
  const float S = std::min(0.2f * node->LODLevel(), 1.0f);
#else
  const float S = 1.0f;
#endif

  // NOTE: These values should come from biome system when implemented
  static constexpr length_t globalMinTerrainHeight = -255 * Block::Length();
  static constexpr length_t globalMaxTerrainHeight = 255 * Block::Length();

  // Number of cells in each direction
  constexpr int cells = Chunk::Size();

  const length_t lodLength = pow2(node->LODLevel()) * Chunk::Length();
  const length_t cellLength = lodLength / cells;

  if (node->anchor.k * Chunk::Length() > globalMaxTerrainHeight || node->anchor.k * Chunk::Length() + lodLength < globalMinTerrainHeight)
    return;

  // Generate heightmap
  length_t minTerrainHeight = std::numeric_limits<length_t>::max();
  length_t maxTerrainHeight = -std::numeric_limits<length_t>::max();
  std::array<std::array<length_t, cells + 1>, cells + 1> terrainHeights{};
  for (int i = 0; i < cells + 1; ++i)
    for (int j = 0; j < cells + 1; ++j)
    {
      Vec2 pointXY = Chunk::Length() * static_cast<Vec2>(node->anchor) + cellLength * Vec2(i, j);
      length_t terrainHeight = terrainFunction(pointXY);
      terrainHeights[i][j] = terrainHeight;

      if (terrainHeight > maxTerrainHeight)
        maxTerrainHeight = terrainHeight;
      if (terrainHeight < minTerrainHeight)
        minTerrainHeight = terrainHeight;
    }

  if (node->anchor.k * Chunk::Length() > maxTerrainHeight || node->anchor.k * Chunk::Length() + lodLength < minTerrainHeight)
    return;

  // Generate mesh
  std::vector<LOD::Vertex> LODMesh{};
  for (int i = 0; i < cells; ++i)
    for (int j = 0; j < cells; ++j)
      for (int k = 0; k < cells; ++k)
      {
        const length_t cellAnchorZPos = node->anchor.k * Chunk::Length() + k * cellLength;

        // Determine which of the 256 cases the cell belongs to
        uint8_t cellCase = 0;
        for (int v = 0; v < 8; ++v)
        {
          // Cell vertex indices and z-position
          int I = v & bit(0) ? i + 1 : i;
          int J = v & bit(1) ? j + 1 : j;
          length_t Z = v & bit(2) ? cellAnchorZPos + cellLength : cellAnchorZPos;

          if (terrainHeights[I][J] > Z)
            cellCase |= bit(v);
        }

        // Use lookup table to determine which of 16 equivalence classes the cell belongs to
        uint8_t cubeEquivClass = regularCellClass[cellCase];
        RegularCellData cellData = regularCellData[cubeEquivClass];
        int triangleCount = cellData.getTriangleCount();

        // Loop over all triangles in cell
        for (int tri = 0; tri < triangleCount; ++tri)
        {
          // Local positions of vertices within LOD
          std::array<Vec3, 3> vertexPositions{};

          for (int v = 0; v < 3; ++v)
          {
            // Lookup indices of vertices A,B of the cell edge that vertex v lies on
            int cellEdgeIndex = cellData.vertexIndex[3 * tri + v];
            uint16_t vertexData = regularVertexData[cellCase][cellEdgeIndex];
            uint8_t indexA = vertexData & 0x000F;
            uint8_t indexB = (vertexData & 0x00F0) >> 4;

            // Normalized vertex positions
            Vec3 vertA = cellVertexOffsets[indexA];
            Vec3 vertB = cellVertexOffsets[indexB];

            // Cell vertex indices and z-positions
            int iA = indexA & bit(0) ? i + 1 : i;
            int iB = indexB & bit(0) ? i + 1 : i;
            int jA = indexA & bit(1) ? j + 1 : j;
            int jB = indexB & bit(1) ? j + 1 : j;
            length_t zA = indexA & bit(2) ? cellAnchorZPos + cellLength : cellAnchorZPos;
            length_t zB = indexB & bit(2) ? cellAnchorZPos + cellLength : cellAnchorZPos;

            // Isovalues of vertices A and B
            float tA = terrainHeights[iA][jA] - zA;
            float tB = terrainHeights[iB][jB] - zB;

            float t = tA / (tA - tB);

            // Calculate the position of the vertex on the cell edge.  The smoothness parameter S is used to interpolate between 
            // roughest iso-surface (vertex is always chosen at edge midpoint) and the smooth iso-surface interpolation used by Paul Bourke.
            Vec3 edgeOffset = vertA + (0.5 * (1 - S) + t * S) * (vertB - vertA);
            vertexPositions[v] = cellLength * Vec3(i, j, k) + cellLength * edgeOffset;
          }

          // Calculate vertex normal
          Vec3 normal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));
          float lightValue = static_cast<float>((2.0 + normal.z) / 3);

          for (int v = 0; v < 3; ++v)
            LODMesh.push_back({ vertexPositions[v], v, lightValue });
        }
      }

  // Generate vertex array
  Engine::Shared<Engine::VertexArray> LODVertexArray = Engine::VertexArray::Create();
  auto LODVertexBuffer = Engine::VertexBuffer::Create(static_cast<uint32_t>(LODMesh.size()) * sizeof(LOD::Vertex));
  LODVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                               { ShaderDataType::Int,    "a_QuadIndex"},
                               { ShaderDataType::Float,  "s_LightValue"} });
  LODVertexArray->addVertexBuffer(LODVertexBuffer);

  uintptr_t dataSize = sizeof(LOD::Vertex) * LODMesh.size();
  LODVertexBuffer->setData(LODMesh.data(), dataSize);

  node->data->vertexArray = LODVertexArray;
  node->data->meshSize = static_cast<uint32_t>(LODMesh.size());
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

length_t ChunkManager::terrainFunction(const Vec2& pointXY) const
{
  length_t Octave1 = 150 * Block::Length() * glm::simplex(pointXY / 1280.0 / Block::Length());
  length_t Octave2 = 50 * Block::Length() * glm::simplex(pointXY / 320.0 / Block::Length());
  length_t Octave3 = 5 * Block::Length() * glm::simplex(pointXY / 40.0 / Block::Length());

  return Octave1 + Octave2 + Octave3;
}
