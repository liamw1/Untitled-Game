#include "GMpch.h"
#include "ChunkSystem.h"
#include "Player/Player.h"
#include <iostream>

ChunkManager::ChunkManager()
{
  m_ChunkArray = new Chunk[s_MaxChunks];

  m_OpenChunkSlots.reserve(s_MaxChunks);
  for (int i = s_MaxChunks - 1; i >= 0; --i)
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
  static constexpr length_t chunkSphereRadius = Constants::SQRT3 * Chunk::Length() / 2;
  for (Vec4& plane : frustumPlanes)
  {
    length_t planeNormalMag = glm::length(Vec3(plane));
    plane.w += chunkSphereRadius * planeNormalMag;
  }

  // Render chunks in view frustum
  Chunk::BindBuffers();
  for (const auto& [key, chunk] : m_RenderableChunks)
    if (isInRange(chunk->getGlobalIndex(), s_RenderDistance))
      if (isInFrustum(chunk->center(), frustumPlanes))
        chunk->draw();
  for (const auto& [key, chunk] : m_UpdateList)
    if (!chunk->isEmpty() && isInRange(chunk->getGlobalIndex(), s_RenderDistance))
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
  if (m_ChunksLoaded == 0)
    loadChunk(Player::OriginIndex());

  // Find new chunks to generate
  std::vector<GlobalIndex> newChunks{};
  for (ChunkMap::iterator it = m_BoundaryChunks.begin(); it != m_BoundaryChunks.end(); ++it)
  {
    const Chunk* chunk = it->second;

    for (Block::Face face : Block::FaceIterator())
    {
      GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face);
      if (isInRange(neighborIndex, s_LoadDistance))
        if (!isLoaded(neighborIndex) && !chunk->isFaceOpaque(face))
          newChunks.push_back(neighborIndex);

      if (newChunks.size() >= maxNewChunks)
        break;
    }
  }

  // Load newly found chunks
  for (const GlobalIndex& newChunkIndex : newChunks)
  {
    if (!isLoaded(newChunkIndex))
      loadChunk(newChunkIndex);

    // If there are no open chunk slots, don't load any more
    if (m_OpenChunkSlots.size() == 0)
      break;
  }

  // Move chunks out of m_BoundaryChunks when all their neighbors are accounted for
  for (ChunkMap::iterator it = m_BoundaryChunks.begin(); it != m_BoundaryChunks.end();)
  {
    const Chunk* chunk = it->second;

    if (!isOnBoundary(chunk))
    {
      for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j)
          for (int k = -1; k <= 1; ++k)
          {
            Chunk* neighbor = find(chunk->getGlobalIndex() + GlobalIndex(i, j, k));
            if (neighbor && neighbor != chunk)
              queueForUpdating(neighbor);
          }

      it = moveToGroup(it, ChunkType::Boundary, ChunkType::NeedsUpdate);
    }
    else
      ++it;
  }

  return newChunks.size();
}

bool ChunkManager::updateChunks(int maxUpdates)
{
  EN_PROFILE_FUNCTION();

  int updatedChunks = 0;
  while (m_UpdateList.begin() != m_UpdateList.end() && updatedChunks <= maxUpdates)
  {
    updateChunk(m_UpdateList.begin());
    updatedChunks++;
  }

  return updatedChunks > 0;
}

void ChunkManager::clean()
{
  EN_PROFILE_FUNCTION();

  // Destroy boundary chunks outside of unload range
  std::vector<GlobalIndex> unloadedChunks{};
  for (ChunkMap::iterator it = m_BoundaryChunks.begin(); it != m_BoundaryChunks.end();)
  {
    const Chunk* chunk = it->second;
    if (!isInRange(chunk->getGlobalIndex(), s_UnloadDistance))
    {
      unloadedChunks.push_back(chunk->getGlobalIndex());
      it = unloadChunk(it);
    }
    else
      ++it;
  }

  for (const GlobalIndex& index : unloadedChunks)
    sendChunkRemovalUpdate(index);

  // Destroy heightmaps outside of unload range
  for (auto it = m_HeightMaps.begin(); it != m_HeightMaps.end();)
  {
    const HeightMap& heightMap = it->second;
    if (!isInRange(GlobalIndex(heightMap.chunkI, heightMap.chunkJ, Player::OriginIndex().k), s_UnloadDistance))
      it = m_HeightMaps.erase(it);
    else
      ++it;
  }
}

Chunk* ChunkManager::find(const LocalIndex& chunkIndex)
{
  const GlobalIndex& originChunk = Player::OriginIndex();
  return find(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

const Chunk* ChunkManager::find(const LocalIndex& chunkIndex) const
{
  const GlobalIndex& originChunk = Player::OriginIndex();
  return find(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void ChunkManager::placeBlock(Chunk* chunk, BlockIndex blockIndex, Block::Face face, Block::Type blockType)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  EN_ASSERT(!chunk->isEmpty(), "Place block cannot be called on an empty chunk!");

  // If trying to place an air block or trying to place a block in a space occupied by another block, do nothing
  if (blockType == Block::Type::Air || !blockNeighborIsAir(chunk, blockIndex, face))
  {
    EN_WARN("Invalid block placement!");
    return;
  }

  if (Chunk::BlockNeighborIsInAnotherChunk(blockIndex, face))
  {
    chunk = findNeighbor(chunk, face);

    if (!chunk)
      return;

    blockIndex -= static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::OutwardNormal(face);
  }
  else
    blockIndex += BlockIndex::OutwardNormal(face);

  chunk->setBlockType(blockIndex, blockType);
  sendBlockUpdate(chunk, blockIndex);
}

void ChunkManager::removeBlock(Chunk* chunk, const BlockIndex& blockIndex)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  EN_ASSERT(chunk->getBlockType(blockIndex) != Block::Type::Air, "Cannot remove air block!");

  chunk->setBlockType(blockIndex, Block::Type::Air);
  sendBlockUpdate(chunk, blockIndex);
}

ChunkManager::ChunkMap::iterator ChunkManager::loadChunk(const GlobalIndex& chunkIndex)
{
  EN_ASSERT(m_OpenChunkSlots.size() > 0, "All chunks slots are full!");
  EN_ASSERT(!isLoaded(chunkIndex), "Chunk is already loaded!");

  // Generate heightmap is none exists
  int heightMapKey = createHeightMapKey(chunkIndex.i, chunkIndex.j);
  auto it = m_HeightMaps.find(heightMapKey);
  if (it == m_HeightMaps.end())
  {
    const auto& [insertionPosition, insertionSuccess] = m_HeightMaps.emplace(heightMapKey, chunkIndex);
    it = insertionPosition;
    EN_ASSERT(insertionSuccess, "HeightMap insertion failed!");
  }
  const HeightMap& heightMap = it->second;

  // Grab first open chunk slot
  int chunkSlot = m_OpenChunkSlots.back();
  m_OpenChunkSlots.pop_back();

  // Insert chunk into array and load it
  m_ChunkArray[chunkSlot] = std::move(Chunk(chunkIndex));
  Chunk* newChunk = &m_ChunkArray[chunkSlot];
  const auto& [insertionPosition, insertionSuccess] = m_BoundaryChunks.insert({ createKey(chunkIndex), newChunk });
  EN_ASSERT(insertionSuccess, "Chunk insertion failed!");
  fillChunk(newChunk, heightMap);
  m_ChunksLoaded++;

  return insertionPosition;
}

ChunkManager::ChunkMap::iterator ChunkManager::updateChunk(ChunkMap::iterator iteratorPosition)
{
  EN_ASSERT(iteratorPosition != m_UpdateList.end(), "End of iterator!");

  Chunk* chunk = iteratorPosition->second;
  chunk->update();

  if (chunk->isEmpty())
    return moveToGroup(iteratorPosition, ChunkType::NeedsUpdate, ChunkType::Empty);

  meshChunk(chunk);
  if (chunk->m_QuadCount == 0 && chunk->getBlockType(0, 0, 0) == Block::Type::Air)
  {
    chunk->clear();
    return moveToGroup(iteratorPosition, ChunkType::NeedsUpdate, ChunkType::Empty);
  }
  else
    return moveToGroup(iteratorPosition, ChunkType::NeedsUpdate, ChunkType::Renderable);
}

ChunkManager::ChunkMap::iterator ChunkManager::unloadChunk(ChunkMap::iterator erasePosition)
{
  EN_ASSERT(erasePosition != m_BoundaryChunks.end(), "End of iterator!");

  const Chunk* chunk = erasePosition->second;

  // Open up chunk slot
  int chunkSlot = static_cast<int>(chunk - &m_ChunkArray[0]);
  EN_ASSERT(&m_ChunkArray[chunkSlot] == chunk, "Calculated index does not correspond to given pointer!");
  m_OpenChunkSlots.push_back(chunkSlot);

  // Delete chunk data
  m_ChunkArray[chunkSlot].reset();

  m_ChunksLoaded--;
  return m_BoundaryChunks.erase(erasePosition);
}

void ChunkManager::sendChunkRemovalUpdate(const GlobalIndex& chunkIndex)
{
  // Move neighbors to m_BoundaryChunks
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* neighbor = find(chunkIndex + GlobalIndex::OutwardNormal(face));

    if (neighbor)
    {
      ChunkType type = getChunkType(neighbor);

      if (type != ChunkType::Boundary)
        moveToGroup(neighbor, type, ChunkType::Boundary);
    }
  }
}

void ChunkManager::queueForUpdating(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  ChunkType type = getChunkType(chunk);
  if (type != ChunkType::NeedsUpdate && type != ChunkType::Boundary)
    moveToGroup(chunk, type, ChunkType::NeedsUpdate);
}

void ChunkManager::updateImmediately(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  ChunkType source = getChunkType(chunk);

  if (source == ChunkType::Boundary && isOnBoundary(chunk))
    return;

  if (source != ChunkType::NeedsUpdate)
    moveToGroup(chunk, source, ChunkType::NeedsUpdate);

  updateChunk(m_UpdateList.find(createKey(chunk->getGlobalIndex())));
}

void ChunkManager::sendBlockUpdate(Chunk* chunk, const BlockIndex& blockIndex)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  updateImmediately(chunk);

  std::vector<Block::Face> updateDirections{};
  for (Block::Face face : Block::FaceIterator())
    if (Chunk::BlockNeighborIsInAnotherChunk(blockIndex, face))
      updateDirections.push_back(face);

  EN_ASSERT(updateDirections.size() <= 3, "Too many update directions!");

  // Update neighbors in cardinal directions
  for (Block::Face face : updateDirections)
  {
    Chunk* neighbor = findNeighbor(chunk, face);
    if (neighbor)
      updateImmediately(neighbor);
    else if (!chunk->isFaceOpaque(face))
    {
      ChunkMap::iterator it = loadChunk(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face));
      updateImmediately(it->second);
    }
  }

  // Queue edge neighbor for updating
  if (updateDirections.size() == 2)
  {
    Chunk* edgeNeighbor = findNeighbor(chunk, updateDirections[0], updateDirections[1]);
    if (edgeNeighbor)
      queueForUpdating(edgeNeighbor);
  }

  // Queue edge/corner neighbors for updating
  if (updateDirections.size() == 3)
  {
    for (int i = 0; i < 3; ++i)
    {
      int j = (i + 1) % 3;
      Chunk* edgeNeighbor = findNeighbor(chunk, updateDirections[i], updateDirections[j]);
      if (edgeNeighbor)
        queueForUpdating(edgeNeighbor);
    }

    Chunk* cornerNeighbor = findNeighbor(chunk, updateDirections[0], updateDirections[1], updateDirections[2]);
    if (cornerNeighbor)
      queueForUpdating(cornerNeighbor);
  }
}

void ChunkManager::fillChunk(Chunk* chunk, const HeightMap& heightMap)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  length_t chunkFloor = Chunk::Length() * chunk->getGlobalIndex().k;
  length_t chunkCeiling = chunkFloor + Chunk::Length();

  chunk->m_NonOpaqueFaces = 0x3F;

  if (chunkFloor > heightMap.maxHeight)
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
      const length_t& terrainHeight = heightMap.surfaceData[i][j].getHeight();
      const Block::Type& surfaceBlockType = heightMap.surfaceData[i][j].getPrimaryBlockType();

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
        int terrainHeightIndex = static_cast<int>((terrainHeight - chunkFloor) / Block::Length());
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
  int faceID = static_cast<int>(face);
  int u = faceID / 2;
  int v = (u + 1) % 3;
  int w = (u + 2) % 3;

  blockIndex += BlockIndex::OutwardNormal(face);
  return blockIndex[u]     < 0 || blockIndex[u]     >= Chunk::Size() ||
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
  if (!chunk)
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
              blockTexID textureID = static_cast<blockTexID>(Block::GetTexture(chunk->getBlockType(i, j, k), face));
              int faceID = static_cast<int>(face);
              int u = faceID / 2;
              int v = (u + 1) % 3;
              int w = (u + 2) % 3;

              for (int vert = 0; vert < 4; ++vert)
              {
                Block::Face sideADir = static_cast<Block::Face>(2 * v + offsets[faceID][vert][v]);
                Block::Face sideBDir = static_cast<Block::Face>(2 * w + offsets[faceID][vert][w]);

                BlockIndex sideA = BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face) + BlockIndex::OutwardNormal(sideADir);
                BlockIndex sideB = BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face) + BlockIndex::OutwardNormal(sideBDir);
                BlockIndex corner = BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face) + BlockIndex::OutwardNormal(sideADir) + BlockIndex::OutwardNormal(sideBDir);

                bool sideAIsOpaque = !(needsNeighbors ? blockHasTransparency(sideA, neighbors) : Block::HasTransparency(chunk->getBlockType(sideA)));
                bool sideBIsOpaque = !(needsNeighbors ? blockHasTransparency(sideB, neighbors) : Block::HasTransparency(chunk->getBlockType(sideB)));
                bool cornerIsOpaque = !(needsNeighbors ? blockHasTransparency(corner, neighbors) : Block::HasTransparency(chunk->getBlockType(corner)));
                int AO = sideAIsOpaque && sideBIsOpaque ? 0 : 3 - (sideAIsOpaque + sideBIsOpaque + cornerIsOpaque);

                BlockIndex vertexIndex = BlockIndex(i, j, k) + offsets[faceID][vert];

                uint32_t vertexData = vertexIndex.i + (vertexIndex.j << 6) + (vertexIndex.k << 12);   // Local vertex coordinates
                vertexData |= vert << 18;                                                             // Quad vertex index
                vertexData |= AO << 20;                                                               // Ambient occlusion value
                vertexData |= textureID << 22;                                                        // TextureID

                meshData[4 * quadCount + vert] = vertexData;
              }
              quadCount++;
            }
          }
        }

  chunk->setMesh(meshData, quadCount);
}

Chunk* ChunkManager::find(const GlobalIndex& chunkIndex)
{
  for (ChunkMap& chunkGroup : m_Chunks)
  {
    ChunkMap::iterator  it = chunkGroup.find(createKey(chunkIndex));

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

const Chunk* ChunkManager::find(const GlobalIndex& chunkIndex) const
{
  for (const ChunkMap& chunkGroup : m_Chunks)
  {
    ChunkMap::const_iterator  it = chunkGroup.find(createKey(chunkIndex));

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

Chunk* ChunkManager::findNeighbor(Chunk* chunk, Block::Face face)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face);
  return find(neighborIndex);
}

Chunk* ChunkManager::findNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(faceA) + GlobalIndex::OutwardNormal(faceB);
  return find(neighborIndex);
}

Chunk* ChunkManager::findNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB, Block::Face faceC)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(faceA) + GlobalIndex::OutwardNormal(faceB) + GlobalIndex::OutwardNormal(faceC);
  return find(neighborIndex);
}

const Chunk* ChunkManager::findNeighbor(const Chunk* chunk, Block::Face face) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face);
  return find(neighborIndex);
}

bool ChunkManager::isOnBoundary(const Chunk* chunk) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  // Cardinal neighbors
  for (Block::Face face : Block::FaceIterator())
    if (!isLoaded(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face)) && !chunk->isFaceOpaque(face))
      return true;
  return false;
}

bool ChunkManager::isLoaded(const GlobalIndex& chunkIndex) const
{
  for (const ChunkMap& chunkGroup : m_Chunks)
    if (chunkGroup.find(createKey(chunkIndex)) != chunkGroup.end())
      return true;
  return false;
}

ChunkManager::ChunkType ChunkManager::getChunkType(const Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist");

  for (auto chunkGroup = m_Chunks.begin(); chunkGroup != m_Chunks.end(); ++chunkGroup)
    if (chunkGroup->find(createKey(chunk->getGlobalIndex())) != chunkGroup->end())
    {
      ptrdiff_t chunkGroupIndex = chunkGroup - m_Chunks.begin();
      return static_cast<ChunkType>(chunkGroupIndex);
    }

  EN_ERROR("Could not find chunk!");
  return ChunkType::Error;
}

void ChunkManager::addToGroup(Chunk* chunk, ChunkType destination)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  EN_ASSERT(getChunkType(chunk) != destination, "Chunk is already of the destination type!");

  int destinationTypeID = static_cast<int>(destination);
  m_Chunks[destinationTypeID].insert({ createKey(chunk->getGlobalIndex()), chunk });
}

ChunkManager::ChunkMap::iterator ChunkManager::moveToGroup(ChunkMap::iterator iteratorPosition, ChunkType source, ChunkType destination)
{
  EN_ASSERT(getChunkType(iteratorPosition->second) == source, "Chunk is not of the source type!");

  int sourceTypeID = static_cast<int>(source);
  int destinationTypeID = static_cast<int>(destination);

  EN_ASSERT(sourceTypeID != destinationTypeID, "Source and destination are the same!");
  EN_ASSERT(iteratorPosition != m_Chunks[sourceTypeID].end(), "End of iterator!");
  EN_ASSERT(!(source == ChunkType::Boundary && destination != ChunkType::NeedsUpdate), "Boundary chunks can only be moved to update list!");

  addToGroup(iteratorPosition->second, destination);
  return m_Chunks[sourceTypeID].erase(iteratorPosition);
}

void ChunkManager::moveToGroup(Chunk* chunk, ChunkType source, ChunkType destination)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  moveToGroup(m_Chunks[static_cast<int>(source)].find(createKey(chunk->getGlobalIndex())), source, destination);
}

bool ChunkManager::blockNeighborIsAir(const Chunk* chunk, const BlockIndex& blockIndex, Block::Face face) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  if (Chunk::BlockNeighborIsInAnotherChunk(blockIndex, face))
  {
    const Chunk* chunkNeighbor = findNeighbor(chunk, face);

    if (!chunkNeighbor)
      return false;
    else if (chunkNeighbor->isEmpty())
      return true;
    else
      return chunkNeighbor->getBlockType(blockIndex - static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::OutwardNormal(face)) == Block::Type::Air;
  }
  else if (chunk->isEmpty())
    return true;
  else
    return chunk->getBlockType(blockIndex + BlockIndex::OutwardNormal(face)) == Block::Type::Air;
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