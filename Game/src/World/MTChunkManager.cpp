#include "GMpch.h"
#include "MTChunkManager.h"
#include "Player/Player.h"
#include "Terrain.h"
#include "Util/Util.h"

MTChunkManager::MTChunkManager()
{
  m_ChunkArray = new Chunk[s_MaxChunks];

  std::vector<int> stackData;
  stackData.reserve(s_MaxChunks);
  m_OpenChunkSlots = std::stack<int, std::vector<int>>(stackData);
  for (int i = s_MaxChunks - 1; i >= 0; --i)
    m_OpenChunkSlots.push(i);

  std::thread loadThread(&MTChunkManager::loadThread, this);
  std::thread updateThread(&MTChunkManager::updateThread, this);

  loadThread.detach();
  updateThread.detach();
}

void MTChunkManager::render()
{
  EN_PROFILE_FUNCTION();

  const Mat4& viewProjection = Engine::Scene::ActiveCameraViewProjection();
  std::array<Vec4, 6> frustumPlanes = Util::CalculateViewFrustumPlanes(viewProjection);

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
  static constexpr length_t chunkSphereRadius = Constants::SQRT3 * Chunk::Length() / 2;
  for (Vec4& plane : frustumPlanes)
  {
    length_t planeNormalMag = glm::length(Vec3(plane));
    plane.w += chunkSphereRadius * planeNormalMag;
  }

  // Render chunks in view frustum
  Chunk::BindBuffers();
  forEach(ChunkType::Renderable, [&](Chunk& chunk)
    {
      if (Util::IsInRangeOfPlayer(chunk, s_RenderDistance) && Util::IsInFrustum(chunk.center(), frustumPlanes))
      {
        std::lock_guard lock = chunk.acquireLock();
        chunk.draw2();
      }
    });
}

void MTChunkManager::loadThread()
{
  using namespace std::chrono_literals;

  while (true)
  {
    mapType<int, GlobalIndex> newChunkPositions;
    {
      forEach(ChunkType::Boundary, [&](const Chunk& chunk)
        {
          for (Block::Face face : Block::FaceIterator())
          {
            GlobalIndex neighborIndex = chunk.getGlobalIndex() + GlobalIndex::OutwardNormal(face);
            if (Util::IsInRangeOfPlayer(neighborIndex, s_LoadDistance) && newChunkPositions.find(Util::CreateKey(neighborIndex)) == newChunkPositions.end())
              if (!chunk.isFaceOpaque(face) && !isLoaded(neighborIndex))
                newChunkPositions.insert({ Util::CreateKey(neighborIndex), neighborIndex });
          }
        });
    }

    // Load First chunk if none exist
    if (m_OpenChunkSlots.size() == s_MaxChunks)
      newChunkPositions.insert({ Util::CreateKey(Player::OriginIndex()), Player::OriginIndex() });

    if (newChunkPositions.size() > 0)
    {
      for (const auto& [key, newChunkIndex] : newChunkPositions)
      {
        Chunk chunk(newChunkIndex);
        Terrain::GenerateNew(&chunk);
        insert(std::move(chunk));
        // EN_INFO("Boundary Chunks: {0}, Renderable Chunks: {1}, Empty Chunks: {2}", m_BoundaryChunks.size(), m_RenderableChunks.size(), m_EmptyChunks.size());
      }
    }
    else
      std::this_thread::sleep_for(1ms);
  }
}

void MTChunkManager::updateThread()
{
  using namespace std::chrono_literals;

  while (true)
  {
    EN_PROFILE_SCOPE("Chunk Updating");

    GlobalIndex updateIndex = m_UpdateQueue.waitAndRemoveOne();
    std::vector<uint32_t> mesh = createMesh(updateIndex);
    update(updateIndex, mesh);
  }
}

bool MTChunkManager::insert(Chunk&& chunk)
{
  std::lock_guard lock(m_ChunkMapMutex);

  EN_ASSERT(!m_OpenChunkSlots.empty(), "All chunks slots are full!");

  // Grab first open chunk slot
  int chunkSlot = m_OpenChunkSlots.top();
  m_OpenChunkSlots.pop();

  // Insert chunk into array and load it
  m_ChunkArray[chunkSlot] = std::move(chunk);
  Chunk* newChunk = &m_ChunkArray[chunkSlot];
  auto [insertionPosition, insertionSuccess] = m_BoundaryChunks.insert({Util::CreateKey(newChunk->getGlobalIndex()), newChunk});

  if (insertionSuccess)
    sendChunkLoadUpdate(newChunk);

  return insertionSuccess;
}

bool MTChunkManager::erase(const GlobalIndex& chunkIndex)
{
  std::lock_guard lock(m_ChunkMapMutex);

  mapType<int, Chunk*>::iterator erasePosition = m_BoundaryChunks.find(Util::CreateKey(chunkIndex));
  if (erasePosition == m_BoundaryChunks.end())
    return false;

  const Chunk* chunk = erasePosition->second;
  std::lock_guard chunkLock = chunk->acquireLock();

  // Open up chunk slot
  int chunkSlot = static_cast<int>(chunk - &m_ChunkArray[0]);
  m_OpenChunkSlots.push(chunkSlot);

  // Delete chunk data
  m_ChunkArray[chunkSlot].reset();
  m_BoundaryChunks.erase(erasePosition);
  return true;
}

bool MTChunkManager::update(const GlobalIndex& chunkIndex, const std::vector<uint32_t>& mesh)
{
  std::lock_guard lock(m_ChunkMapMutex);

  Chunk* chunk = find(chunkIndex);
  if (!chunk)
    return false;

  ChunkType source = getChunkType(chunk);
  if (source == ChunkType::Boundary)
    return false;

  std::lock_guard chunkLock = chunk->acquireLock();
  chunk->determineOpacity();
  chunk->m_Mesh = mesh;
  if (!chunk->isEmpty() && mesh.empty() && chunk->getBlockType(0, 0, 0) == Block::Type::Air) // Maybe should be part of SetMesh
    chunk->clear();

  ChunkType destination = chunk->isEmpty() ? ChunkType::Empty : ChunkType::Renderable;
  if (source != destination)
    recategorizeChunk(chunk, source, destination);

  return true;
}

static Block::Type getBlockType(const Block::Type* blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k)
{
  EN_ASSERT(blockData, "Block data does not exist!");
  EN_ASSERT(-1 <= i && i <= Chunk::Size() && -1 <= j && j <= Chunk::Size() && -1 <= k && k <= Chunk::Size(), "Index is out of bounds!");
  return blockData[(Chunk::Size() + 2) * (Chunk::Size() + 2) * (i + 1) + (Chunk::Size() + 2) * (j + 1) + (k + 1)];
}

static Block::Type getBlockType(const Block::Type* blockData, const BlockIndex& blockIndex)
{
  return getBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k);
}

static void setBlockType(Block::Type* blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(blockData, "Block data does not exist!");
  EN_ASSERT(-1 <= i && i <= Chunk::Size() && -1 <= j && j <= Chunk::Size() && -1 <= k && k <= Chunk::Size(), "Index is out of bounds!");
  blockData[(Chunk::Size() + 2) * (Chunk::Size() + 2) * (i + 1) + (Chunk::Size() + 2) * (j + 1) + (k + 1)] = blockType;
}

static void setBlockType(Block::Type* blockData, const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

std::vector<uint32_t> MTChunkManager::createMesh(const GlobalIndex& chunkIndex) const
{
  static constexpr int totalVolumeNeeded = (Chunk::Size() + 2) * (Chunk::Size() + 2) * (Chunk::Size() + 2);
  thread_local Block::Type* const blockData = new Block::Type[totalVolumeNeeded];

  for (int i = 0; i < totalVolumeNeeded; ++i)
    blockData[i] = Block::Type::Null;

  {
    std::shared_lock lock(m_ChunkMapMutex);

    // Load blocks from chunk
    const Chunk* chunk = find(chunkIndex);
    if (chunk)
    {
      std::lock_guard chunkLock = chunk->acquireLock();

      if (chunk->isEmpty())
        return {};

      for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
        for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
          for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
            setBlockType(blockData, i, j, k, chunk->getBlockType(i, j, k));
    }
    else
      return {};

    // Load blocks from cardinal neighbors
    for (Block::Face face : Block::FaceIterator())
    {
      int faceIndex = IsPositive(face) ? Chunk::Size() : -1;
      int neighborFaceIndex = IsPositive(face) ? 0 : Chunk::Size() - 1;

      const Chunk* neighbor = findNeighbor(chunkIndex, face);
      if (neighbor)
      {
        std::lock_guard chunkLock = neighbor->acquireLock();

        for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
          for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
          {
            BlockIndex relativeBlockIndex = BlockIndex::CreatePermuted(faceIndex, i, j, GetCoordID(face));

            if (neighbor->isEmpty())
              setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
            else
            {
              BlockIndex neighborBlockIndex = BlockIndex::CreatePermuted(neighborFaceIndex, i, j, GetCoordID(face));
              setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
            }
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

        const Chunk* neighbor = findNeighbor(chunkIndex, faceA, faceB);
        if (neighbor)
        {
          std::lock_guard lock = neighbor->acquireLock();

          for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
          {
            BlockIndex relativeBlockIndex{};
            relativeBlockIndex[u] = IsPositive(faceA) ? Chunk::Size() : -1;
            relativeBlockIndex[v] = IsPositive(faceB) ? Chunk::Size() : -1;
            relativeBlockIndex[w] = i;

            if (neighbor->isEmpty())
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
      }

    // Load blocks from corner neighbors
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        for (int k = 0; k < 2; ++k)
        {
          GlobalIndex neighborIndex = chunkIndex + GlobalIndex(i, j, k) - GlobalIndex(1 - i, 1 - j, 1 - k);
          const Chunk* neighbor = find(neighborIndex);
          if (neighbor)
          {
            std::lock_guard lock = neighbor->acquireLock();

            BlockIndex relativeBlockIndex = Chunk::Size() * BlockIndex(i, j, k) - BlockIndex(1 - i, 1 - j, 1 - k);

            if (neighbor->isEmpty())
              setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
            else
            {
              BlockIndex neighborBlockIndex = static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex(1 - i, 1 - j, 1 - k);
              setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
            }
          }
        }
  }

  static constexpr BlockIndex offsets[6][4]
    = { { {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1} },    /*  West Face   */
        { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },    /*  East Face   */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /*  South Face  */
        { {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1} },    /*  North Face  */
        { {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0} },    /*  Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} } };  /*  Top Face    */

  EN_PROFILE_FUNCTION();

  // Mesh chunk using block data
  std::vector<uint32_t> mesh;
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

                mesh.push_back(vertexData);
              }
            }
      }
  return mesh;
}

void MTChunkManager::forEach(ChunkType chunkType, const std::function<void(Chunk& chunk)>& func) const
{
  std::shared_lock lock(m_ChunkMapMutex);

  for (const auto& [key, chunk] : m_Chunks[static_cast<int>(chunkType)])
    func(*chunk);
}

std::vector<GlobalIndex> MTChunkManager::findAll(ChunkType chunkType, bool (*condition)(const Chunk& chunk)) const
{
  std::vector<GlobalIndex> indexList;

  std::shared_lock lock(m_ChunkMapMutex);

  for (const auto& [key, chunk] : m_Chunks[static_cast<int>(chunkType)])
    if (condition(*chunk))
      indexList.push_back(chunk->getGlobalIndex());
  return indexList;
}

bool MTChunkManager::IndexSet::add(const GlobalIndex& index)
{
  std::lock_guard lock(m_Mutex);

  auto [insertionPosition, insertionSuccess] = m_Data.insert({ Util::CreateKey(index), index });
  m_DataCondition.notify_one();
  return insertionSuccess;
}

GlobalIndex MTChunkManager::IndexSet::waitAndRemoveOne()
{
  std::unique_lock lock(m_Mutex);
  m_DataCondition.wait(lock, [this] { return !m_Data.empty(); });

  GlobalIndex value = m_Data.begin()->second;
  m_Data.erase(m_Data.begin());
  return value;
}



bool MTChunkManager::isLoaded(const GlobalIndex& chunkIndex) const
{
  for (const mapType<int, Chunk*>& chunkGroup : m_Chunks)
    if (chunkGroup.find(Util::CreateKey(chunkIndex)) != chunkGroup.end())
      return true;
  return false;
}

bool MTChunkManager::isOnBoundary(const Chunk* chunk) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  for (Block::Face face : Block::FaceIterator())
    if (!isLoaded(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face)) && !chunk->isFaceOpaque(face))
      return true;
  return false;
}

MTChunkManager::ChunkType MTChunkManager::getChunkType(const Chunk* chunk) const
{
  EN_ASSERT(chunk, "Chunk does not exist");

  for (int chunkType = 0; chunkType < s_ChunkTypes; ++chunkType)
    if (m_Chunks[chunkType].find(Util::CreateKey(chunk)) != m_Chunks[chunkType].end())
      return static_cast<ChunkType>(chunkType);

  EN_ERROR("Could not find chunk!");
  return ChunkType::Error;
}

Chunk* MTChunkManager::find(const GlobalIndex& chunkIndex)
{
  for (mapType<int, Chunk*>& chunkGroup : m_Chunks)
  {
    mapType<int, Chunk*>::iterator it = chunkGroup.find(Util::CreateKey(chunkIndex));

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

const Chunk* MTChunkManager::find(const GlobalIndex& chunkIndex) const
{
  for (const mapType<int, Chunk*>& chunkGroup : m_Chunks)
  {
    mapType<int, Chunk*>::const_iterator it = chunkGroup.find(Util::CreateKey(chunkIndex));

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

const Chunk* MTChunkManager::findNeighbor(const GlobalIndex& chunkIndex, Block::Face face) const
{
  return find(chunkIndex + GlobalIndex::OutwardNormal(face));
}

const Chunk* MTChunkManager::findNeighbor(const GlobalIndex& chunkIndex, Block::Face faceA, Block::Face faceB) const
{
  return find(chunkIndex + GlobalIndex::OutwardNormal(faceA) + GlobalIndex::OutwardNormal(faceB));
}

const Chunk* MTChunkManager::findNeighbor(const GlobalIndex& chunkIndex, Block::Face faceA, Block::Face faceB, Block::Face faceC) const
{
  return find(chunkIndex + GlobalIndex::OutwardNormal(faceA) + GlobalIndex::OutwardNormal(faceB) + GlobalIndex::OutwardNormal(faceC));
}

void MTChunkManager::sendChunkLoadUpdate(Chunk* newChunk)
{
  boundaryChunkUpdate(newChunk);

  // Move cardinal neighbors out of m_BoundaryChunks if they are no longer on boundary
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* neighbor = find(newChunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face));

    if (!neighbor)  // TODO: Change this
      continue;

    if (getChunkType(neighbor) == ChunkType::Boundary)
      boundaryChunkUpdate(neighbor);
  }
}

void MTChunkManager::boundaryChunkUpdate(Chunk* chunk)
{
  EN_ASSERT(getChunkType(chunk) == ChunkType::Boundary, "Chunk is not a boundary chunk!");

  if (!isOnBoundary(chunk))
  {
    ChunkType destination = chunk->isEmpty() ? ChunkType::Empty : ChunkType::Renderable;
    recategorizeChunk(chunk, ChunkType::Boundary, destination);
  }
}

void MTChunkManager::recategorizeChunk(Chunk* chunk, ChunkType source, ChunkType destination)
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
          GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex(i, j, k);
          if (isLoaded(neighborIndex))
            m_UpdateQueue.add(neighborIndex);
        }

  int key = Util::CreateKey(chunk);
  int sourceTypeID = static_cast<int>(source);
  int destinationTypeID = static_cast<int>(destination);

  m_Chunks[destinationTypeID].insert({ key, chunk });
  m_Chunks[sourceTypeID].erase(m_Chunks[sourceTypeID].find(key));
}