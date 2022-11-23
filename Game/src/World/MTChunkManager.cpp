#include "GMpch.h"
#include "MTChunkManager.h"
#include "Player/Player.h"
#include "Terrain.h"
#include "Util/Util.h"

MTChunkManager::MTChunkManager()
  : m_Running(true)
{
  m_ChunkArray = new Chunk[s_MaxChunks];

  std::vector<int> stackData;
  stackData.reserve(s_MaxChunks);
  m_OpenChunkSlots = std::stack<int, std::vector<int>>(stackData);
  for (int i = s_MaxChunks - 1; i >= 0; --i)
    m_OpenChunkSlots.push(i);
}

MTChunkManager::~MTChunkManager()
{
  m_Running.store(false);
  if (m_LoadThread.joinable())
    m_LoadThread.join();
  if (m_UpdateThread.joinable())
    m_UpdateThread.join();

  delete[] m_ChunkArray;
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

        if (!chunk.m_Mesh.empty())
        {
          chunk.setMesh(chunk.m_Mesh.data(), static_cast<uint16_t>(chunk.m_Mesh.size() / 4));
          chunk.m_Mesh.clear();
        }

        chunk.draw();
      }
    });
}

void MTChunkManager::update()
{
  EN_PROFILE_FUNCTION();

  while (true)
  {
    std::optional<GlobalIndex> updateIndex = m_ForceUpdateQueue.tryRemove();
    if (updateIndex)
    {
      bool loaded;
      {
        std::shared_lock lock(m_ChunkMapMutex);
        loaded = isLoaded(*updateIndex);
      }

      // TODO: Consolidate these into separate functions
      if (!loaded)
        loadNewChunk(*updateIndex);

      std::vector<uint32_t> mesh = createMesh(*updateIndex);
      update(*updateIndex, mesh);
    }
    else
      break;
  }
}

void MTChunkManager::clean()
{
  EN_PROFILE_FUNCTION();

  std::vector<GlobalIndex> chunksMarkedForDeletion = findAll(ChunkType::Boundary, [](const Chunk& chunk)
    {
      return !Util::IsInRangeOfPlayer(chunk, s_UnloadDistance);
    });

  if (!chunksMarkedForDeletion.empty())
  {
    for (const GlobalIndex& chunkIndex : chunksMarkedForDeletion)
      if (!Util::IsInRangeOfPlayer(chunkIndex, s_UnloadDistance))
        erase(chunkIndex);
    Terrain::Clean(s_UnloadDistance);
  }
}

std::pair<const Chunk*, std::unique_lock<std::mutex>> MTChunkManager::acquireChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return acquireChunk(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void MTChunkManager::placeBlock(const GlobalIndex& chunkIndex, BlockIndex blockIndex, Block::Face face, Block::Type blockType)
{
  {
    auto [chunk, lock] = acquireChunk(chunkIndex);

    if (!chunk || chunk->getBlockType(blockIndex) == Block::Type::Air)
    {
      EN_WARN("Cannot call placeBlock on an air block!");
      return;
    }

    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, face))
    {
      auto [neighbor, neighborLock] = acquireChunk(chunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face));

      if (!neighbor)
        return;

      chunk = neighbor;
      lock = std::move(neighborLock);

      blockIndex -= static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::OutwardNormal(face);
    }
    else
      blockIndex += BlockIndex::OutwardNormal(face);

    // If trying to place an air block or trying to place a block in a space occupied by another block, do nothing
    if (blockType == Block::Type::Air || (!chunk->isEmpty() && chunk->getBlockType(blockIndex) != Block::Type::Air))
    {
      EN_WARN("Invalid block placement!");
      return;
    }

    chunk->setBlockType(blockIndex, blockType);
  }

  sendBlockUpdate(chunkIndex, blockIndex);
}

void MTChunkManager::removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  {
    auto [chunk, lock] = acquireChunk(chunkIndex);
    chunk->setBlockType(blockIndex, Block::Type::Air);
  }

  sendBlockUpdate(chunkIndex, blockIndex);
}

void MTChunkManager::loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType)
{
  Block::Type* composition = new Block::Type[Chunk::TotalBlocks()];
  for (int i = 0; i < Chunk::TotalBlocks(); ++i)
    composition[i] = blockType;

  Chunk newChunk(chunkIndex);
  newChunk.setData(composition);
  insert(std::move(newChunk));
}



void MTChunkManager::loadWorker()
{
  using namespace std::chrono_literals;

  while (m_Running.load())
  {
    EN_PROFILE_SCOPE("Load worker");

    setType<GlobalIndex> newChunkPositions;
    forEach(ChunkType::Boundary, [&](const Chunk& chunk)
      {
        for (Block::Face face : Block::FaceIterator())
        {
          GlobalIndex neighborIndex = chunk.getGlobalIndex() + GlobalIndex::OutwardNormal(face);
          if (Util::IsInRangeOfPlayer(neighborIndex, s_LoadDistance) && newChunkPositions.find(neighborIndex) == newChunkPositions.end())
            if (!chunk.isFaceOpaque(face) && !isLoaded(neighborIndex))
              newChunkPositions.insert(neighborIndex);
        }
      });

    // Load First chunk if none exist
    if (m_OpenChunkSlots.size() == s_MaxChunks)
      newChunkPositions.insert(Player::OriginIndex());

    if (!newChunkPositions.empty())
      for (const GlobalIndex& newChunkIndex : newChunkPositions)
        loadNewChunk(newChunkIndex);
    else
      std::this_thread::sleep_for(100ms);
  }
}

void MTChunkManager::updateWorker()
{
  using namespace std::chrono_literals;

  while (m_Running.load())
  {
    EN_PROFILE_SCOPE("Update worker");

    std::optional<GlobalIndex> updateIndex = m_LazyUpdateQueue.tryRemove();
    if (updateIndex)
    {
      std::vector<uint32_t> mesh = createMesh(*updateIndex);
      update(*updateIndex, mesh);
    }
    else
      std::this_thread::sleep_for(100ms);
  }
}

void MTChunkManager::loadNewChunk(const GlobalIndex& chunkIndex)
{
  Chunk chunk(chunkIndex);
  m_LoadTerrain ? Terrain::GenerateNew(&chunk) : Terrain::GenerateEmpty(&chunk);
  insert(std::move(chunk));
}



bool MTChunkManager::insert(Chunk&& chunk)
{
  std::lock_guard lock(m_ChunkMapMutex);

  if (m_OpenChunkSlots.empty())
    return false;

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

  sendChunkRemovalUpdate(chunkIndex);

  return true;
}

bool MTChunkManager::update(const GlobalIndex& chunkIndex, const std::vector<uint32_t>& mesh)
{
  std::shared_lock sharedLock(m_ChunkMapMutex);

  Chunk* chunk = find(chunkIndex);
  if (!chunk)
    return false;

  ChunkType source = getChunkType(chunk);
  if (source == ChunkType::Boundary)
    return false;

  std::lock_guard chunkLock = chunk->acquireLock();
  sharedLock.unlock();

  chunk->determineOpacity();
  chunk->m_Mesh = mesh;
  if (!chunk->isEmpty() && chunk->m_Mesh.empty() && chunk->getBlockType(0, 0, 0) == Block::Type::Air) // Maybe should be part of SetMesh
    chunk->clear();

  ChunkType destination = chunk->isEmpty() ? ChunkType::Empty : ChunkType::Renderable;
  if (source != destination)
  {
    std::lock_guard lock(m_ChunkMapMutex);
    recategorizeChunk(chunk, source, destination);
  }

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

  // Load blocks from chunk
  {
    auto [chunk, lock] = acquireChunk(chunkIndex);
    if (chunk)
    {
      if (chunk->isEmpty())
        return {};

      for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
        for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
          for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
            setBlockType(blockData, i, j, k, chunk->getBlockType(i, j, k));
    }
    else
      return {};
  }

  // Load blocks from cardinal neighbors
  for (Block::Face face : Block::FaceIterator())
  {
    int faceIndex = IsPositive(face) ? Chunk::Size() : -1;
    int neighborFaceIndex = IsPositive(face) ? 0 : Chunk::Size() - 1;

    auto [neighbor, lock] = acquireChunk(chunkIndex + GlobalIndex::OutwardNormal(face));
    if (neighbor)
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

      auto [neighbor, lock] = acquireChunk(chunkIndex + GlobalIndex::OutwardNormal(faceA) + GlobalIndex::OutwardNormal(faceB));
      if (neighbor)
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

  // Load blocks from corner neighbors
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
      {
        GlobalIndex neighborIndex = chunkIndex + GlobalIndex(i, j, k) - GlobalIndex(1 - i, 1 - j, 1 - k);
        auto [neighbor, lock] = acquireChunk(neighborIndex);
        if (neighbor)
        {
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

  static constexpr BlockIndex offsets[6][4]
    = { { {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1} },    /*  West Face   */
        { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },    /*  East Face   */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /*  South Face  */
        { {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1} },    /*  North Face  */
        { {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0} },    /*  Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} } };  /*  Top Face    */

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

std::pair<Chunk*, std::unique_lock<std::mutex>> MTChunkManager::acquireChunk(const GlobalIndex& chunkIndex)
{
  std::shared_lock lock(m_ChunkMapMutex);
  std::unique_lock<std::mutex> chunkLock;

  Chunk* chunk = find(chunkIndex);
  if (chunk)
    chunkLock = std::unique_lock(chunk->m_Mutex);

  return { chunk, std::move(chunkLock) };
}

std::pair<const Chunk*, std::unique_lock<std::mutex>> MTChunkManager::acquireChunk(const GlobalIndex& chunkIndex) const
{
  std::shared_lock lock(m_ChunkMapMutex);
  std::unique_lock<std::mutex> chunkLock;

  const Chunk* chunk = find(chunkIndex);
  if (chunk)
    chunkLock = std::unique_lock(chunk->m_Mutex);

  return { chunk, std::move(chunkLock) };
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

std::optional<GlobalIndex> MTChunkManager::IndexSet::tryRemove()
{
  std::lock_guard lock(m_Mutex);

  if (!m_Data.empty())
  {
    GlobalIndex value = m_Data.begin()->second;
    m_Data.erase(m_Data.begin());
    return value;
  }
  return std::nullopt;
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

void MTChunkManager::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  m_ForceUpdateQueue.add(chunkIndex);

  std::vector<Block::Face> updateDirections{};
  for (Block::Face face : Block::FaceIterator())
    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, face))
      updateDirections.push_back(face);

  EN_ASSERT(updateDirections.size() <= 3, "Too many update directions for a single block update!");

  // Update neighbors in cardinal directions
  for (Block::Face face : updateDirections)
  {
    GlobalIndex neighborIndex = chunkIndex + GlobalIndex::OutwardNormal(face);
    m_ForceUpdateQueue.add(neighborIndex);
  }

  // Queue edge neighbor for updating
  if (updateDirections.size() == 2)
  {
    GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::OutwardNormal(updateDirections[0]) + GlobalIndex::OutwardNormal(updateDirections[1]);
    m_LazyUpdateQueue.add(edgeNeighborIndex);
  }

  // Queue edge/corner neighbors for updating
  if (updateDirections.size() == 3)
  {
    GlobalIndex cornerNeighborIndex = chunkIndex;
    for (int i = 0; i < 3; ++i)
    {
      int j = (i + 1) % 3;
      GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::OutwardNormal(updateDirections[i]) + GlobalIndex::OutwardNormal(updateDirections[j]);
      m_LazyUpdateQueue.add(edgeNeighborIndex);

      cornerNeighborIndex += GlobalIndex::OutwardNormal(updateDirections[i]);
    }

    m_LazyUpdateQueue.add(cornerNeighborIndex);
  }
}

void MTChunkManager::sendChunkLoadUpdate(Chunk* newChunk)
{
  boundaryChunkUpdate(newChunk);

  // Move cardinal neighbors out of m_BoundaryChunks if they are no longer on boundary
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* neighbor = find(newChunk->getGlobalIndex() + GlobalIndex::OutwardNormal(face));
    if (neighbor && getChunkType(neighbor) == ChunkType::Boundary)
      boundaryChunkUpdate(neighbor);
  }
}

void MTChunkManager::sendChunkRemovalUpdate(const GlobalIndex& removalIndex)
{
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* neighbor = find(removalIndex + GlobalIndex::OutwardNormal(face));

    if (neighbor)
    {
      ChunkType type = getChunkType(neighbor);

      if (type != ChunkType::Boundary)
        recategorizeChunk(neighbor, type, ChunkType::Boundary);
    }
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
            m_LazyUpdateQueue.add(neighborIndex);
        }

  int key = Util::CreateKey(chunk);
  int sourceTypeID = static_cast<int>(source);
  int destinationTypeID = static_cast<int>(destination);

  m_Chunks[destinationTypeID].insert({ key, chunk });
  m_Chunks[sourceTypeID].erase(m_Chunks[sourceTypeID].find(key));
}