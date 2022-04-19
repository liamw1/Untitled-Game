#include "GMpch.h"
#include "ChunkManager.h"
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
    if (isInRange(chunk, s_RenderDistance))
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

  searchForNewChunks();

  // Load newly found chunks
  int chunksLoaded = 0;
  for (IndexMap::iterator it = m_NewChunkList.begin(); it != m_NewChunkList.end() && chunksLoaded < maxNewChunks && m_OpenChunkSlots.size() > 0;)
  {
    const GlobalIndex& newChunkIndex = it->second;

    loadChunk(newChunkIndex);
    chunksLoaded++;

    it = m_NewChunkList.erase(it);
  }

  return chunksLoaded;
}

bool ChunkManager::updateChunks(int maxUpdates)
{
  EN_PROFILE_FUNCTION();

  int updatedChunks = 0;
  for (IndexMap::iterator it = m_UpdateList.begin(); it != m_UpdateList.end() && updatedChunks < maxUpdates;)
  {
    const GlobalIndex& updateIndex = it->second;
    Chunk* chunk = find(updateIndex);

    if (chunk)
    {
      it = updateChunk(chunk);
      updatedChunks++;
    }
    else
      it = m_UpdateList.erase(it);
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

    if (!isInRange(chunk, s_UnloadDistance))
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

void ChunkManager::searchForNewChunks()
{
  EN_PROFILE_FUNCTION();

  for (const auto& [key, chunk] : m_BoundaryChunks)
    for (Block::Face face : Block::FaceIterator())
    {
      GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face);
      if (isInRange(neighborIndex, s_LoadDistance) && m_NewChunkList.find(createKey(neighborIndex)) == m_NewChunkList.end())
        if (!chunk->isFaceOpaque(face) && !isLoaded(neighborIndex))
          m_NewChunkList.insert({ createKey(neighborIndex), neighborIndex });
    }
}

Chunk* ChunkManager::loadChunk(const GlobalIndex& chunkIndex)
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

  newChunk->fill(heightMap);
  m_ChunksLoaded++;

  sendChunkLoadUpdate(newChunk);

  return newChunk;
}

ChunkManager::IndexMap::iterator ChunkManager::updateChunk(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  EN_ASSERT(m_UpdateList.find(createKey(chunk)) != m_UpdateList.end(), "Chunks is not in update list!");

  ChunkType source = getChunkType(chunk);
  if (source != ChunkType::Boundary)
  {
    chunk->update();

    ChunkType destination;
    if (chunk->isEmpty())
      destination = ChunkType::Empty;
    else
    {
      meshChunk(chunk);

      if (chunk->getQuadCount() == 0 && chunk->getBlockType(0, 0, 0) == Block::Type::Air)
      {
        chunk->clear();
        destination = ChunkType::Empty;
      }
      else
        destination = ChunkType::Renderable;
    }

    if (source != destination)
      moveToGroup(chunk, source, destination);
  }

  return m_UpdateList.erase(m_UpdateList.find(createKey(chunk)));
}

ChunkManager::ChunkMap::iterator ChunkManager::unloadChunk(ChunkMap::iterator erasePosition)
{
  EN_ASSERT(erasePosition != m_BoundaryChunks.end(), "End of iterator!");

  const Chunk* chunk = erasePosition->second;

  // Open up chunk slot
  int chunkSlot = static_cast<int>(chunk - &m_ChunkArray[0]);
  m_OpenChunkSlots.push_back(chunkSlot);

  // Delete chunk data
  m_ChunkArray[chunkSlot].reset();
  m_ChunksLoaded--;

  return m_BoundaryChunks.erase(erasePosition);
}

void ChunkManager::boundaryChunkUpdate(Chunk* chunk)
{
  EN_ASSERT(getChunkType(chunk) == ChunkType::Boundary, "Chunk is not a boundary chunk!");

  if (!isOnBoundary(chunk))
  {
    ChunkType destination = chunk->isEmpty() ? ChunkType::Empty : ChunkType::Renderable;
    moveToGroup(chunk, ChunkType::Boundary, destination);
  }
}

void ChunkManager::sendChunkLoadUpdate(Chunk* newChunk)
{
  boundaryChunkUpdate(newChunk);

  // Move cardinal neighbors out of m_BoundaryChunks if they are no longer on boundary
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* neighbor = findNeighbor(newChunk, face);

    if (!neighbor)
      continue;

    if (getChunkType(neighbor) == ChunkType::Boundary)
      boundaryChunkUpdate(neighbor);
  }
}

void ChunkManager::sendChunkRemovalUpdate(const GlobalIndex& chunkIndex)
{
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

void ChunkManager::sendBlockUpdate(Chunk* chunk, const BlockIndex& blockIndex)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  updateImmediately(chunk);

  std::vector<Block::Face> updateDirections{};
  for (Block::Face face : Block::FaceIterator())
    if (Chunk::BlockNeighborIsInAnotherChunk(blockIndex, face))
      updateDirections.push_back(face);

  EN_ASSERT(updateDirections.size() <= 3, "Too many update directions for a single block update!");

  // Update neighbors in cardinal directions
  for (Block::Face face : updateDirections)
  {
    Chunk* neighbor = findNeighbor(chunk, face);
    if (neighbor)
      updateImmediately(neighbor);
    else if (!chunk->isFaceOpaque(face))
    {
      Chunk* newChunk = loadChunk(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face));
      updateImmediately(newChunk);
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

void ChunkManager::queueForUpdating(const Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  m_UpdateList.insert({ createKey(chunk), chunk->getGlobalIndex() });
}

void ChunkManager::updateImmediately(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  queueForUpdating(chunk);
  updateChunk(chunk);
}

static Block::Type getBlockType(Block::Type* blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k)
{
  return blockData[(Chunk::Size() + 2) * (Chunk::Size() + 2) * (i + 1) + (Chunk::Size() + 2) * (j + 1) + (k + 1)];
}

static Block::Type getBlockType(Block::Type* blockData, const BlockIndex& blockIndex)
{
  return getBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k);
}

static void setBlockType(Block::Type* blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  blockData[(Chunk::Size() + 2) * (Chunk::Size() + 2) * (i + 1) + (Chunk::Size() + 2) * (j + 1) + (k + 1)] = blockType;
}

static void setBlockType(Block::Type* blockData, const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

void ChunkManager::meshChunk(Chunk* chunk)
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  static constexpr BlockIndex offsets[6][4]
    = { { {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1} },    /*  West Face   */
        { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },    /*  East Face   */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /*  South Face  */
        { {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1} },    /*  North Face  */
        { {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0} },    /*  Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} } };  /*  Top Face    */

  // NOTE: Should probably replace with custom memory allocation system
  static constexpr int maxVertices = 4 * 6 * Chunk::TotalBlocks();
  static uint32_t* const meshData = new uint32_t[maxVertices];

  static constexpr int totalBlocksNeeded = (Chunk::Size() + 2) * (Chunk::Size() + 2) * (Chunk::Size() + 2);
  static Block::Type* const blockData = new Block::Type[totalBlocksNeeded];

  if (chunk->isEmpty())
    return;

  EN_PROFILE_FUNCTION();

  // Load blocks from chunk
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
        setBlockType(blockData, i, j, k, chunk->getBlockType(i, j, k));

  // Load blocks from cardinal neighbors
  for (Block::Face face : Block::FaceIterator())
  {
    int faceIndex = IsPositive(face) ? Chunk::Size() : -1;
    int neighborFaceIndex = IsPositive(face) ? 0 : Chunk::Size() - 1;

    Chunk* neighbor = findNeighbor(chunk, face);
    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        BlockIndex relativeBlockIndex = BlockIndex::CreatePermuted(faceIndex, i, j, GetCoordID(face));

        if (!neighbor)
          setBlockType(blockData, relativeBlockIndex, Block::Type::Null);
        else if (neighbor->isEmpty())
          setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
        else
        {
          BlockIndex neighborBlockIndex = BlockIndex::CreatePermuted(neighborFaceIndex, i, j, GetCoordID(face));
          setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
        }
      }
  }

  // Load blocks from edge neighbors
  for (auto itA = Block::FaceIterator().begin(); itA != Block::FaceIterator().end(); ++itA)
    for (auto itB = itA.next(); itB != Block::FaceIterator().end(); ++itB)
    {
      Block::Face faceA = *itA;
      Block::Face faceB = *itB;

      // Opposite faces cannot form edge
      if (faceB == !faceA)
        continue;

      int u = GetCoordID(faceA);
      int v = GetCoordID(faceB);
      int w = (2 * (u + v)) % 3;  // Extracts coordID that runs along edge

      Chunk* neighbor = findNeighbor(chunk, faceA, faceB);
      for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      {
        BlockIndex relativeBlockIndex{};
        relativeBlockIndex[u] = IsPositive(faceA) ? Chunk::Size() : -1;
        relativeBlockIndex[v] = IsPositive(faceB) ? Chunk::Size() : -1;
        relativeBlockIndex[w] = i;

        if (!neighbor)
          setBlockType(blockData, relativeBlockIndex, Block::Type::Null);
        else if (neighbor->isEmpty())
          setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
        else
        {
          BlockIndex neighborBlockIndex{};
          neighborBlockIndex[u] = IsPositive(faceA) ? 0 : Chunk::Size() - 1;
          neighborBlockIndex[v] = IsPositive(faceB) ? 0 : Chunk::Size() - 1;
          neighborBlockIndex[w] = i;

          setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
        }
      }
    }

  // Load blocks from corner neighbors
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
      {
        GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex(i, j, k) - GlobalIndex(1 - i, 1 - j, 1 - k);
        Chunk* neighbor = find(neighborIndex);

        BlockIndex relativeBlockIndex = Chunk::Size() * BlockIndex(i, j, k) - BlockIndex(1 - i, 1 - j, 1 - k);
        if (!neighbor)
          setBlockType(blockData, relativeBlockIndex, Block::Type::Null);
        else if (neighbor->isEmpty())
          setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
        else
        {
          BlockIndex neighborBlockIndex = static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex(1 - i, 1 - j, 1 - k);
          setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
        }
      }

  // Mesh chunk using block data
  int quadCount = 0;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
      {
        Block::Type blockType = getBlockType(blockData, i, j, k);

        if (blockType != Block::Type::Air)
          for (Block::Face face : Block::FaceIterator())
            if (Block::HasTransparency(getBlockType(blockData, BlockIndex(i, j, k) + BlockIndex::OutwardNormal(face))))
            {
              int textureID = static_cast<int>(Block::GetTexture(blockType, face));
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

                bool sideAIsOpaque = !Block::HasTransparency(getBlockType(blockData, sideA));
                bool sideBIsOpaque = !Block::HasTransparency(getBlockType(blockData, sideB));
                bool cornerIsOpaque = !Block::HasTransparency(getBlockType(blockData, corner));
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
  return find(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face));
}

Chunk* ChunkManager::findNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  return find(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(faceA) + GlobalIndex::OutwardNormal(faceB));
}

Chunk* ChunkManager::findNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB, Block::Face faceC)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  return find(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(faceA) + GlobalIndex::OutwardNormal(faceB) + GlobalIndex::OutwardNormal(faceC));
}

const Chunk* ChunkManager::findNeighbor(const Chunk* chunk, Block::Face face) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  return find(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face));
}

bool ChunkManager::isOnBoundary(const Chunk* chunk) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");

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
    if (chunkGroup->find(createKey(chunk)) != chunkGroup->end())
    {
      ptrdiff_t chunkGroupIndex = chunkGroup - m_Chunks.begin();
      return static_cast<ChunkType>(chunkGroupIndex);
    }

  EN_ERROR("Could not find chunk!");
  return ChunkType::Error;
}

void ChunkManager::moveToGroup(Chunk* chunk, ChunkType source, ChunkType destination)
{
  EN_ASSERT(chunk, "Chunk does not exist!");
  EN_ASSERT(source != destination, "Source and destination are the same!");
  EN_ASSERT(getChunkType(chunk) == source, "Chunk is not of the source type!");

  // Chunks moved from m_BoundaryChunks get queued for updating, along with all their neighbors
  if (source == ChunkType::Boundary)
    for (int i = -1; i <= 1; ++i)
      for (int j = -1; j <= 1; ++j)
        for (int k = -1; k <= 1; ++k)
        {
          Chunk* neighbor = find(chunk->getGlobalIndex() + GlobalIndex(i, j, k));
          if (neighbor)
            queueForUpdating(neighbor);
        }

  int key = createKey(chunk);
  int sourceTypeID = static_cast<int>(source);
  int destinationTypeID = static_cast<int>(destination);

  m_Chunks[destinationTypeID].insert({ key, chunk });
  m_Chunks[sourceTypeID].erase(m_Chunks[sourceTypeID].find(key));
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

int ChunkManager::createKey(const Chunk* chunk) const
{
  return createKey(chunk->getGlobalIndex());
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

bool ChunkManager::isInRange(const Chunk* chunk, globalIndex_t range) const
{
  return isInRange(chunk->getGlobalIndex(), range);
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