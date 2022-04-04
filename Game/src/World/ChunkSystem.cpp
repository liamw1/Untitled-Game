#include "GMpch.h"
#include "ChunkSystem.h"
#include "Player/Player.h"
#include <iostream>

static constexpr int s = sizeof(Chunk);

template<typename T>
static bool isIn(const std::vector<T>& arr, const T* element)
{
#if GM_EXPERIMENTAL
  if (arr.begin() == arr.end())
    return false;

  const T* arrBegin = &(*arr.begin());
  const T* arrEnd = &*(arr.end() - 1);

  return element >= arrBegin && element <= arrEnd;
#else
  return std::binary_search(arr.begin(), arr.end(), *element);
#endif
}

template<typename T>
static bool isIn(const std::vector<T>& arr, const T& element)
{
  return isIn(arr, &element);
}

template<typename T>
static bool isIn(const std::vector<T>& arr, const GlobalIndex& chunkIndex)
{
  return std::binary_search(arr.begin(), arr.end(), chunkIndex);
}

template<typename T>
static T* findIn(std::vector<T>& arr, const GlobalIndex& chunkIndex)
{
  auto it = std::lower_bound(arr.begin(), arr.end(), chunkIndex);
  return it == arr.end() || (*it != chunkIndex) ? nullptr : &(*it);
}

template<typename T>
static const T* findIn(const std::vector<T>& arr, const GlobalIndex& chunkIndex)
{
  auto it = std::lower_bound(arr.begin(), arr.end(), chunkIndex);
  return it == arr.end() || (*it != chunkIndex) ? nullptr : &(*it);
}



void ChunkManager::initialize()
{
  while (loadNewChunks(10000))
  {
  }

  // initializeLODs();
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
  for (const Chunk& chunk : m_RenderableChunks)
    if (isInRange(chunk.getGlobalIndex(), s_RenderDistance))
      if (isInFrustum(chunk.center(), frustumPlanes))
        chunk.draw();
}

bool ChunkManager::loadNewChunks(int maxNewChunks)
{
  EN_PROFILE_FUNCTION();

  // Load First chunk if none exist
  if (m_ChunksLoaded == 0)
    loadChunk(Player::OriginIndex());

  // Find new chunks to generate
  int newChunks = 0;
  for (auto chunk = m_BoundaryChunks.begin(); chunk != m_BoundaryChunks.end(); ++chunk)
    for (Block::Face face : Block::FaceIterator())
    {
      const GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face);
      if (isInRange(neighborIndex, s_LoadDistance))
        if (!isLoaded(neighborIndex) && !chunk->isFaceOpaque(face))
        {
          chunk = loadChunk(neighborIndex);
          newChunks++;
        }

      if (newChunks >= maxNewChunks)
        break;
    }

  // Move chunks out of m_BoundaryChunks when all their neighbors are accounted for
  for (auto chunk = m_BoundaryChunks.begin(); chunk != m_BoundaryChunks.end();)
  {
    if (!isOnBoundary(chunk))
    {
      meshChunk(chunk);

      ChunkType destination = chunk->isEmpty() ? ChunkType::Empty : ChunkType::Renderable;
      chunk = moveToGroup(chunk, ChunkType::Boundary, destination);
    }
    else
      ++chunk;
  }

  return newChunks;
}

void ChunkManager::clean()
{
  EN_PROFILE_FUNCTION();

  EN_ASSERT(m_BoundaryChunks.size() > 0, "No boundary chunks exist!");

  // Destroy boundary chunks outside of unload range
  std::vector<Chunk*> chunksToRemove{};
  for (auto chunk = m_BoundaryChunks.begin(); chunk != m_BoundaryChunks.end();)
    if (!isInRange(chunk->getGlobalIndex(), s_UnloadDistance))
      chunk = unloadChunk(*chunk);
    else
      ++chunk;

  // Destroy heightmaps outside of unload range
  for (auto heightMap = m_HeightMaps.begin(); heightMap != m_HeightMaps.end();)
    if (!isInRange(GlobalIndex(heightMap->chunkI, heightMap->chunkJ, Player::OriginIndex().k), s_UnloadDistance))
      heightMap = m_HeightMaps.erase(heightMap);
    else
      ++heightMap;
}

Chunk* ChunkManager::find(const LocalIndex& chunkIndex)
{
  GlobalIndex originChunk = Player::OriginIndex();
  return find(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

const Chunk* ChunkManager::find(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return find(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

static bool isBlockNeighborInAnotherChunk(const BlockIndex& blockIndex, Block::Face face)
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
  const int faceID = static_cast<int>(face);
  const int coordID = faceID / 2;

  return blockIndex[coordID] == chunkLimits[faceID % 2];
}

bool ChunkManager::isBlockNeighborAir(const Chunk* chunk, const BlockIndex& blockIndex, Block::Face face) const
{
  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    const Chunk* chunkNeighbor = getNeighbor(chunk, face);

    if (chunkNeighbor == nullptr)
      return false;
    else if (chunkNeighbor->isEmpty())
      return true;
    else
      return chunkNeighbor->getBlockType(blockIndex - static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::OutwardNormal(face)) == Block::Type::Air;
  }
  else
    return chunk->getBlockType(blockIndex + BlockIndex::OutwardNormal(face)) == Block::Type::Air;
}

void ChunkManager::placeBlock(Chunk* chunk, const BlockIndex& blockIndex, Block::Face face, Block::Type blockType)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  EN_ASSERT(!chunk->isEmpty(), "Place block cannot be called on an empty chunk!");

  // If trying to place an air block or trying to place a block in a space occupied by another block, do nothing
  if (blockType == Block::Type::Air || !isBlockNeighborAir(chunk, blockIndex, face))
    return;

  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    Chunk* chunkNeighbor = getNeighbor(chunk, face);

    if (chunkNeighbor == nullptr)
      return;

    chunkNeighbor->setBlockType(blockIndex - static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::OutwardNormal(face), blockType);
    meshChunk(chunkNeighbor);
  }
  else
    chunk->setBlockType(blockIndex + BlockIndex::OutwardNormal(face), blockType);

  // NOTE: Fix this, should update more neighbors potentially.  Also, send chunk updates
  meshChunk(chunk);
}

void ChunkManager::removeBlock(Chunk* chunk, const BlockIndex& blockIndex)
{
  chunk->setBlockType(blockIndex, Block::Type::Air);

  meshChunk(chunk);
  for (Block::Face face : Block::FaceIterator())
    if (isBlockNeighborInAnotherChunk(blockIndex, face) && getNeighbor(chunk, face) != nullptr)
      meshChunk(getNeighbor(chunk, face));
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

std::vector<Chunk>::iterator ChunkManager::loadChunk(const GlobalIndex& chunkIndex)
{
  EN_ASSERT(!isLoaded(chunkIndex), "Chunk is already loaded!");

  // Generate heightmap is none exists
  HeightMap* heightMap = findIn(m_HeightMaps, chunkIndex);
  if (heightMap == nullptr)
  {
    auto insertionPosition = std::lower_bound(m_HeightMaps.begin(), m_HeightMaps.end(), chunkIndex);
    heightMap = &(*m_HeightMaps.emplace(insertionPosition, chunkIndex.i, chunkIndex.j));
    EN_ASSERT(std::is_sorted(m_HeightMaps.begin(), m_HeightMaps.end()), "Height map was not correctly inserted");
  }
  EN_ASSERT(heightMap, "Heightmap was not found!");

  // Insert chunk into array and load it
  auto insertionPosition = std::lower_bound(m_BoundaryChunks.begin(), m_BoundaryChunks.end(), chunkIndex);
  auto chunk = m_BoundaryChunks.emplace(insertionPosition, chunkIndex);
  fillChunk(chunk, heightMap);

  EN_ASSERT(std::is_sorted(m_BoundaryChunks.begin(), m_BoundaryChunks.end()), "Chunk was not correctly inserted!");

  m_ChunksLoaded++;
  return chunk;

  // Flag neighbors for updating (TODO)
  /*
  for (Block::Face face : Block::FaceIterator())
  {
    GlobalIndex neighborIndex = chunkIndex + GlobalIndex::OutwardNormal(face);

    auto& [found, chunkNeighbor] = find(m_Chunks, neighborIndex);
    if (found)
    {

    }
  }
  */
}

std::vector<Chunk>::iterator ChunkManager::unloadChunk(Chunk& chunk)
{
  EN_ASSERT(isIn(m_BoundaryChunks, chunk), "Chunk is not a boundary chunk!");

  // Move neighbors to m_BoundaryChunks
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* chunkNeighbor = getNeighbor(chunk, face);
    if (chunkNeighbor)
    {
      // Check if chunk is already in m_BoundaryChunk.  If so, skip it
      if (isIn(m_BoundaryChunks, chunkNeighbor))
        continue;

      // Move chunk to m_BoundaryChunks
      ChunkType source = chunkNeighbor->isEmpty() ? ChunkType::Empty : ChunkType::Renderable;
      EN_ASSERT(isIn(m_Chunks[static_cast<int>(source)], chunkNeighbor), "Chunk is not of the correct type!");

      moveToGroup(getIterator(chunkNeighbor, source), source, ChunkType::Boundary);
    }
  }

  // Remove chunk from m_BoundaryChunks
  m_ChunksLoaded--;
  return m_BoundaryChunks.erase(getIterator(&chunk, ChunkType::Boundary));
}

void ChunkManager::fillChunk(Chunk* chunk, const HeightMap* heightMap)
{
  const length_t chunkFloor = Chunk::Length() * chunk->getGlobalIndex().k;
  const length_t chunkCeiling = chunkFloor + Chunk::Length();

  if (chunkFloor > heightMap->maxHeight)
    return;

  Block::Type* chunkComposition = new Block::Type[Chunk::TotalBlocks()];

  auto setBlockType = [=](blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
  {
    chunkComposition[i * Chunk::Size() * Chunk::Size() + j * Chunk::Size() + k] = blockType;
  };

  bool isEmpty = true;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      const length_t& terrainHeight = heightMap->surfaceData[i][j].getHeight();
      const Block::Type& surfaceBlockType = heightMap->surfaceData[i][j].getPrimaryBlockType();

      if (terrainHeight < chunkFloor)
      {
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          setBlockType(i, j, k, Block::Type::Air);
      }
      else if (terrainHeight > chunkCeiling + Block::Length())
      {
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          setBlockType(i, j, k, Block::Type::Dirt);
        isEmpty = false;
      }
      else
      {
        const int terrainHeightIndex = static_cast<int>((terrainHeight - chunkFloor) / Block::Length());
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
        {
          if (k == terrainHeightIndex)
          {
            setBlockType(i, j, k, surfaceBlockType);
            isEmpty = false;
          }
          else if (k < terrainHeightIndex)
          {
            setBlockType(i, j, k, Block::Type::Dirt);
            isEmpty = false;
          }
          else
            setBlockType(i, j, k, Block::Type::Air);
        }
      }
    }

  if (isEmpty)
  {
    delete[] chunkComposition;
    chunkComposition = nullptr;
  }
  else
    chunk->setData(chunkComposition);
}

void ChunkManager::fillChunk(std::vector<Chunk>::iterator chunk, const HeightMap* heightMap)
{
  fillChunk(&(*chunk), heightMap);
}

static bool blockIsOnChunkBorder(const BlockIndex& blockIndex)
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  for (int i = 0; i < 3; ++i)
    if (blockIndex[i] <= chunkLimits[0] || blockIndex[i] >= chunkLimits[1])
      return true;
  return false;
}

static bool blockNeedsChunkNeighbors(const BlockIndex& blockIndex)
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  for (int i = 0; i < 3; ++i)
    if (blockIndex[i] <= chunkLimits[0] + 1 || blockIndex[i] >= chunkLimits[1] - 1)
      return true;
  return false;
}

static bool quadNeedsNeighboringChunks(BlockIndex blockIndex, Block::Face face)
{
  const int faceID = static_cast<int>(face);
  const int u = faceID / 2;
  const int v = (u + 1) % 3;
  const int w = (u + 2) % 3;

  blockIndex += BlockIndex::OutwardNormal(face);
  return blockIndex[u] < 0     || blockIndex[u] >= Chunk::Size()     ||
         blockIndex[v] - 1 < 0 || blockIndex[v] + 1 >= Chunk::Size() ||
         blockIndex[w] - 1 < 0 || blockIndex[w] + 1 >= Chunk::Size();
}

static bool blockHasTransparency(BlockIndex blockIndex, const StackArray3D<const Chunk*, 3>& grid)
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  EN_ASSERT(-Chunk::Size() < blockIndex.i && blockIndex.i < 2 * Chunk::Size() &&
            -Chunk::Size() < blockIndex.j && blockIndex.j < 2 * Chunk::Size() &&
            -Chunk::Size() < blockIndex.k && blockIndex.k < 2 * Chunk::Size(), "Block is more than one chunk away!");

  BlockIndex chunkOffset{};
  for (int i = 0; i < 3; ++i)
  {
    if (blockIndex[i] < chunkLimits[0])
      chunkOffset[i] = -1;
    else if (blockIndex[i] > chunkLimits[1])
      chunkOffset[i] = 1;
  }

  blockIndex -= Chunk::Size() * chunkOffset;
  EN_ASSERT(0 <= blockIndex.i && blockIndex.i < Chunk::Size() &&
            0 <= blockIndex.j && blockIndex.j < Chunk::Size() &&
            0 <= blockIndex.k && blockIndex.k < Chunk::Size(), "Block index is out of bounds!");

  const Chunk* chunk = grid[++chunkOffset.i][++chunkOffset.j][++chunkOffset.k];
  if (chunk == nullptr)
    return false;
  if (chunk->isEmpty())
    return true;

  return Block::HasTransparency(chunk->getBlockType(blockIndex));
}

void ChunkManager::meshChunk(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  // NOTE: Should probably replace with custom memory allocation system
  static constexpr int maxVertices = 4 * 6 * Chunk::TotalBlocks();
  static uint32_t* const meshData = new uint32_t[maxVertices];

  static constexpr BlockIndex offsets[6][4]
    = { { {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1} },    /*  West Face   */
        { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },    /*  East Face   */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /*  South Face  */
        { {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1} },    /*  North Face  */
        { {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0} },    /*  Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} } };  /*  Top Face    */

  if (chunk->isEmpty())
    return;

  EN_PROFILE_FUNCTION();

  StackArray3D<const Chunk*, 3> neighbors{};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k)
        neighbors[i][j][k] = find(chunk->getGlobalIndex() + GlobalIndex(i - 1, j - 1, k - 1));

  int quadCount = 0;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
        if (chunk->getBlockType(i, j, k) != Block::Type::Air)
        {
          bool needsNeighbors = blockNeedsChunkNeighbors(BlockIndex(i, j, k));

          for (Block::Face face : Block::FaceIterator())
          {
            bool blockNeighborHasTransparency = needsNeighbors ? blockHasTransparency(BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face), neighbors)
                                                               : Block::HasTransparency(chunk->getBlockType(BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face)));
            if (blockNeighborHasTransparency)
            {
              const blockTexID textureID = static_cast<blockTexID>(Block::GetTexture(chunk->getBlockType(i, j, k), face));
              const int faceID = static_cast<int>(face);
              const int u = faceID / 2;
              const int v = (u + 1) % 3;
              const int w = (u + 2) % 3;

              for (int vert = 0; vert < 4; ++vert)
              {
                Block::Face sideADir = static_cast<Block::Face>(2 * v + offsets[faceID][vert][v]);
                Block::Face sideBDir = static_cast<Block::Face>(2 * w + offsets[faceID][vert][w]);

                BlockIndex sideA =  BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face) + BlockIndex::OutwardNormal(sideADir);
                BlockIndex sideB =  BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face) + BlockIndex::OutwardNormal(sideBDir);
                BlockIndex corner = BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face) + BlockIndex::OutwardNormal(sideADir) + BlockIndex::OutwardNormal(sideBDir);

                bool sideAIsOpaque =  !(needsNeighbors ? blockHasTransparency(sideA, neighbors)  : Block::HasTransparency(chunk->getBlockType(sideA)));
                bool sideBIsOpaque =  !(needsNeighbors ? blockHasTransparency(sideB, neighbors)  : Block::HasTransparency(chunk->getBlockType(sideB)));
                bool cornerIsOpaque = !(needsNeighbors ? blockHasTransparency(corner, neighbors) : Block::HasTransparency(chunk->getBlockType(corner)));
                int AO = sideAIsOpaque && sideBIsOpaque ? 0 : 3 - (sideAIsOpaque + sideBIsOpaque + cornerIsOpaque);

                BlockIndex vertexIndex = BlockIndex(i, j, k) + offsets[faceID][vert];

                uint32_t vertexData = vertexIndex.i + (vertexIndex.j << 6) + (vertexIndex.k << 12);   // Local vertex coordinates
                vertexData |= faceID << 18;                                                           // Face index
                vertexData |= vert << 21;                                                             // Quad vertex index
                vertexData |= AO << 23;                                                               // Ambient occlusion value
                vertexData |= textureID << 25;                                                        // TextureID

                meshData[4 * quadCount + vert] = vertexData;
              }
              quadCount++;
            }
          }
        }

  if (quadCount == 0)
    return;

  chunk->setMesh(meshData, quadCount);
}

void ChunkManager::meshChunk(std::vector<Chunk>::iterator chunk)
{
  meshChunk(&(*chunk));
}

Chunk* ChunkManager::getNeighbor(Chunk& chunk, Block::Face face)
{
  GlobalIndex neighborIndex = chunk.getGlobalIndex() + GlobalIndex::OutwardNormal(face);
  return find(neighborIndex);
}

Chunk* ChunkManager::getNeighbor(Chunk* chunk, Block::Face face)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  return getNeighbor(*chunk, face);
}

const Chunk* ChunkManager::getNeighbor(const Chunk& chunk, Block::Face face) const
{
  GlobalIndex neighborIndex = chunk.getGlobalIndex() + GlobalIndex::OutwardNormal(face);
  return find(neighborIndex);
}

const Chunk* ChunkManager::getNeighbor(const Chunk* chunk, Block::Face face) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  return getNeighbor(*chunk, face);
}

bool ChunkManager::isOnBoundary(const Chunk& chunk) const
{
  // Cardinal neighbors
  for (Block::Face face : Block::FaceIterator())
    if (!isLoaded(chunk.getGlobalIndex() + GlobalIndex::OutwardNormal(face)) && !chunk.isFaceOpaque(face))
      return true;

  return false;
}

bool ChunkManager::isOnBoundary(std::vector<Chunk>::iterator chunk) const
{
  return isOnBoundary(*chunk);
}

bool ChunkManager::isLoaded(const GlobalIndex& chunkIndex) const
{
  for (const std::vector<Chunk>& chunkGroup : m_Chunks)
    if (isIn(chunkGroup, chunkIndex))
      return true;
  return false;
}

Chunk* ChunkManager::find(const GlobalIndex& chunkIndex)
{
  for (std::vector<Chunk>& chunkGroup : m_Chunks)
  {
    Chunk* chunk = findIn(chunkGroup, chunkIndex);
    if (chunk)
      return chunk;
  }
  return nullptr;
}

const Chunk* ChunkManager::find(const GlobalIndex& chunkIndex) const
{
  for (const std::vector<Chunk>& chunkGroup : m_Chunks)
  {
    const Chunk* chunk = findIn(chunkGroup, chunkIndex);
    if (chunk)
      return chunk;
  }
  return nullptr;
}

void ChunkManager::addToGroup(Chunk& chunk, ChunkType destination)
{
  const int destinationTypeID = static_cast<int>(destination);

  EN_ASSERT(!isIn(m_Chunks[destinationTypeID], chunk), "Chunks of the destination type!");

  m_Chunks[destinationTypeID].insert(std::lower_bound(m_Chunks[destinationTypeID].begin(), m_Chunks[destinationTypeID].end(), chunk.getGlobalIndex()), std::move(chunk));
}

std::vector<Chunk>::iterator ChunkManager::moveToGroup(std::vector<Chunk>::iterator iteratorPosition, ChunkType source, ChunkType destination)
{
  const int sourceTypeID = static_cast<int>(source);
  const int destinationTypeID = static_cast<int>(destination);

  EN_ASSERT(sourceTypeID != destinationTypeID, "Source and destination are the same!");
  EN_ASSERT(iteratorPosition != m_Chunks[sourceTypeID].end(), "End of iterator!");

  addToGroup(*iteratorPosition, destination);
  return m_Chunks[sourceTypeID].erase(iteratorPosition);
}

std::vector<Chunk>::iterator ChunkManager::getIterator(const Chunk* chunk, ChunkType source)
{
  const int sourceTypeID = static_cast<int>(source);

  EN_ASSERT(chunk, "No chunk exists!");
  EN_ASSERT(isIn(m_Chunks[sourceTypeID], chunk), "Chunk is not of source type!");

#if GM_EXPERIMENTAL
  intptr_t arrayIndex = (chunk - &(*m_Chunks[static_cast<int>(source)].begin()));
  return m_Chunks[sourceTypeID].begin() + arrayIndex;
#else
  return std::lower_bound(m_Chunks[sourceTypeID].begin(), m_Chunks[sourceTypeID].end(), chunk->getGlobalIndex());
#endif
}

std::vector<HeightMap>::iterator ChunkManager::getIterator(const HeightMap& heightMap)
{
#if GM_EXPERIMENTAL
    intptr_t arrayIndex = (&heightMap - &(*m_HeightMaps.begin()));
  return m_HeightMaps.begin() + arrayIndex;
#else
  return std::lower_bound(m_HeightMaps.begin(), m_HeightMaps.end(), heightMap);
#endif
}