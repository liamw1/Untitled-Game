#include "GMpch.h"
#include "ChunkContainer.h"
#include "Player/Player.h"
#include "Util/Util.h"

static constexpr int c_MaxChunks = (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1);

ChunkContainer::ChunkContainer()
{
  m_ChunkArray = std::make_unique<Chunk[]>(c_MaxChunks);

  std::vector<int> stackData;
  stackData.reserve(c_MaxChunks);
  m_OpenChunkSlots = std::stack<int, std::vector<int>>(std::move(stackData));
  for (int i = c_MaxChunks - 1; i >= 0; --i)
    m_OpenChunkSlots.push(i);
}

bool ChunkContainer::insert(Chunk&& newChunk)
{
  std::lock_guard lock(m_ContainerMutex);

  if (m_OpenChunkSlots.empty())
    return false;

  // Grab first open chunk slot
  int chunkSlot = m_OpenChunkSlots.top();
  m_OpenChunkSlots.pop();

  // Insert chunk into array
  Chunk& chunk = m_ChunkArray[chunkSlot];
  chunk = std::move(newChunk);
  auto [insertionPosition, insertionSuccess] = m_BoundaryChunks.insert({ chunk.globalIndex(), &chunk });

  if (insertionSuccess)
    sendChunkLoadUpdate(chunk);

  return insertionSuccess;
}

bool ChunkContainer::erase(const GlobalIndex& chunkIndex)
{
  std::lock_guard lock(m_ContainerMutex);

  mapType<GlobalIndex, Chunk*>::iterator erasePosition = m_BoundaryChunks.find(chunkIndex);
  if (erasePosition == m_BoundaryChunks.end())
    return false;
  Chunk* chunk = erasePosition->second;

  // Open up chunk slot
  int chunkSlot = static_cast<int>(chunk - &m_ChunkArray[0]);
  m_OpenChunkSlots.push(chunkSlot);

  // Delete chunk data
  {
    std::lock_guard chunkLock = chunk->acquireLockGuard();
    m_ChunkArray[chunkSlot].reset();
  }
  m_BoundaryChunks.erase(erasePosition);

  sendChunkRemovalUpdate(chunkIndex);

  return true;
}

bool ChunkContainer::update(const GlobalIndex& chunkIndex, bool meshGenerated)
{
  std::shared_lock sharedLock(m_ContainerMutex);

  auto [chunkType, chunk] = find(chunkIndex);
  if (chunkType != ChunkType::Interior)
    return false;

  std::lock_guard chunkLock = chunk->acquireLockGuard();
  sharedLock.unlock();

  chunk->update(meshGenerated);

  return true;
}

std::unordered_set<GlobalIndex> ChunkContainer::findAllLoadableIndices() const
{
  std::shared_lock lock(m_ContainerMutex);

  GlobalIndex originIndex = Player::OriginIndex();
  std::unordered_set<GlobalIndex> newChunkIndices;
  for (const auto& [key, chunk] : m_BoundaryChunks)
    for (Direction direction : Directions())
    {
      GlobalIndex neighborIndex = chunk->globalIndex() + GlobalIndex::Dir(direction);
      if (Util::IsInRange(neighborIndex, originIndex, c_LoadDistance) && newChunkIndices.find(neighborIndex) == newChunkIndices.end())
        if (!chunk->isFaceOpaque(direction) && !isLoaded(neighborIndex))
          newChunkIndices.insert(neighborIndex);
    }
  return newChunkIndices;
}

void ChunkContainer::uploadMeshes(Engine::Threads::UnorderedSetQueue<Chunk::DrawCommand>& commandQueue, std::unique_ptr<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>& multiDrawArray) const
{
  std::shared_lock lock(m_ContainerMutex);

  std::optional<Chunk::DrawCommand> drawCommand = commandQueue.tryRemove();
  while (drawCommand)
  {
    multiDrawArray->remove(drawCommand->id());
    if (isLoaded(drawCommand->id()))
      multiDrawArray->add(std::move(*drawCommand));

    drawCommand = commandQueue.tryRemove();
  }
}

ChunkWithLock ChunkContainer::acquireChunk(const GlobalIndex& chunkIndex)
{
  ConstChunkWithLock chunkWithLock = static_cast<const ChunkContainer*>(this)->acquireChunk(chunkIndex);
  return { const_cast<Chunk*>(chunkWithLock.chunk), std::move(chunkWithLock.lock) };
}

ConstChunkWithLock ChunkContainer::acquireChunk(const GlobalIndex& chunkIndex) const
{
  std::shared_lock lock(m_ContainerMutex);
  std::unique_lock<std::mutex> chunkLock;

  auto [chunkType, chunk] = find(chunkIndex);
  if (chunk)
    chunkLock = chunk->acquireLock();

  return { chunk, std::move(chunkLock) };
}

void ChunkContainer::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  m_ForceUpdateQueue.add(chunkIndex);

  std::vector<Direction> updateDirections{};
  for (Direction direction : Directions())
    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, direction))
      updateDirections.push_back(direction);

  EN_ASSERT(updateDirections.size() <= 3, "Too many update directions for a single block update!");

  // Update neighbors in cardinal directions
  for (Direction direction : updateDirections)
  {
    GlobalIndex neighborIndex = chunkIndex + GlobalIndex::Dir(direction);
    m_ForceUpdateQueue.add(neighborIndex);
  }

  // Queue edge neighbor for updating
  if (updateDirections.size() == 2)
  {
    GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::Dir(updateDirections[0]) + GlobalIndex::Dir(updateDirections[1]);
    m_LazyUpdateQueue.add(edgeNeighborIndex);
  }

  // Queue edge/corner neighbors for updating
  if (updateDirections.size() == 3)
  {
    GlobalIndex cornerNeighborIndex = chunkIndex;
    for (int i = 0; i < 3; ++i)
    {
      int j = (i + 1) % 3;
      GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::Dir(updateDirections[i]) + GlobalIndex::Dir(updateDirections[j]);
      m_LazyUpdateQueue.add(edgeNeighborIndex);

      cornerNeighborIndex += GlobalIndex::Dir(updateDirections[i]);
    }

    m_LazyUpdateQueue.add(cornerNeighborIndex);
  }
}

std::optional<GlobalIndex> ChunkContainer::getLazyUpdateIndex()
{
  return m_LazyUpdateQueue.tryRemove();
}

std::optional<GlobalIndex> ChunkContainer::getForceUpdateIndex()
{
  return m_ForceUpdateQueue.tryRemove();
}

void ChunkContainer::addToLazyUpdateQueue(const GlobalIndex& chunkIndex)
{
  if (!m_ForceUpdateQueue.contains(chunkIndex))
    m_LazyUpdateQueue.add(chunkIndex);
}

void ChunkContainer::addToForceUpdateQueue(const GlobalIndex& chunkIndex)
{
  m_LazyUpdateQueue.remove(chunkIndex);
  m_ForceUpdateQueue.add(chunkIndex);
}

bool ChunkContainer::empty() const
{
  std::shared_lock lock(m_ContainerMutex);
  return m_OpenChunkSlots.size() == c_MaxChunks;
}

bool ChunkContainer::contains(const GlobalIndex& chunkIndex) const
{
  std::shared_lock lock(m_ContainerMutex);
  return isLoaded(chunkIndex);
}



static GlobalIndex globalIndexOffset(BlockIndex& blockIndex)
{
  EN_ASSERT(boundsCheck(blockIndex.i, -Chunk::Size(), 2 * Chunk::Size()) &&
            boundsCheck(blockIndex.j, -Chunk::Size(), 2 * Chunk::Size()) &&
            boundsCheck(blockIndex.k, -Chunk::Size(), 2 * Chunk::Size()), "Requested block is more than one chunk away!");

  GlobalIndex indexOffset{};
  for (int i = 0; i < 3; ++i)
  {
    if (blockIndex[i] < 0)
    {
      indexOffset[i]--;
      blockIndex[i] += Chunk::Size();
    }
    else if (blockIndex[i] >= Chunk::Size())
    {
      indexOffset[i]++;
      blockIndex[i] -= Chunk::Size();
    }
  }
  return indexOffset;
}

ChunkContainer::Stencil::Stencil(ChunkContainer& chunkContainer, const GlobalIndex& centerChunk)
  : m_ChunkContainer(&chunkContainer), m_CenterChunk(centerChunk) {}

Block::Light ChunkContainer::Stencil::getBlockLight(BlockIndex blockIndex)
{
  GlobalIndex offset = globalIndexOffset(blockIndex);

  const std::optional<ChunkWithLock>& query = chunkQuery(offset);
  if (!query)
    return Block::Light(0);
  if (!query->chunk)
    return Block::Light(Block::Light::MaxValue());
  return query->chunk->getBlockLight(blockIndex);
}

void ChunkContainer::Stencil::setBlockLight(BlockIndex blockIndex, Block::Light blockLight)
{
  GlobalIndex offset = globalIndexOffset(blockIndex);

  std::optional<ChunkWithLock>& query = chunkQuery(offset);
  if (!query || !query->chunk)
    return;
  query->chunk->setBlockLight(blockIndex, blockLight);
}

std::optional<ChunkWithLock>& ChunkContainer::Stencil::chunkQuery(const GlobalIndex& indexOffset)
{
  std::optional<ChunkWithLock>& query = m_Chunks[indexOffset.i + 1][indexOffset.j + 1][indexOffset.k + 1];
  if (!query)
    query = m_ChunkContainer->acquireChunk(m_CenterChunk + indexOffset);
  return query;
}



bool ChunkContainer::isLoaded(const GlobalIndex& chunkIndex) const
{
  for (const mapType<GlobalIndex, Chunk*>& chunkGroup : m_Chunks)
    if (chunkGroup.find(chunkIndex) != chunkGroup.end())
      return true;
  return false;
}

bool ChunkContainer::isOnBoundary(const Chunk& chunk) const
{
  for (Direction direction : Directions())
    if (!isLoaded(chunk.globalIndex() + GlobalIndex::Dir(direction)) && !chunk.isFaceOpaque(direction))
      return true;
  return false;
}

std::pair<ChunkType, Chunk*> ChunkContainer::find(const GlobalIndex& chunkIndex)
{
  auto [chunkType, chunk] = static_cast<const ChunkContainer*>(this)->find(chunkIndex);
  return { chunkType, const_cast<Chunk*>(chunk) };
}

std::pair<ChunkType, const Chunk*> ChunkContainer::find(const GlobalIndex& chunkIndex) const
{
  for (int chunkType = 0; chunkType < m_Chunks.size(); ++chunkType)
  {
    mapType<GlobalIndex, Chunk*>::const_iterator it = m_Chunks[chunkType].find(chunkIndex);

    if (it != m_Chunks[chunkType].end())
      return { static_cast<ChunkType>(chunkType), it->second };
  }
  return { ChunkType::DNE, nullptr };
}

void ChunkContainer::sendChunkLoadUpdate(Chunk& newChunk)
{
  boundaryChunkUpdate(newChunk);

  // Move cardinal neighbors out of m_BoundaryChunks if they are no longer on boundary
  for (Direction direction : Directions())
  {
    auto [neighborType, neighbor] = find(newChunk.globalIndex() + GlobalIndex::Dir(direction));
    if (neighborType == ChunkType::Boundary)
      boundaryChunkUpdate(*neighbor);
  }
}

void ChunkContainer::sendChunkRemovalUpdate(const GlobalIndex& removalIndex)
{
  for (Direction direction : Directions())
  {
    auto [neighborType, neighbor] = find(removalIndex + GlobalIndex::Dir(direction));
    if (neighborType == ChunkType::Interior)
      recategorizeChunk(*neighbor, ChunkType::Interior, ChunkType::Boundary);
  }
}

void ChunkContainer::boundaryChunkUpdate(Chunk& chunk)
{
  EN_ASSERT(find(chunk.globalIndex()).first == ChunkType::Boundary, "Chunk is not a boundary chunk!");

  if (!isOnBoundary(chunk))
  {
    ChunkType destination = ChunkType::Interior;
    recategorizeChunk(chunk, ChunkType::Boundary, destination);
  }
}

void ChunkContainer::recategorizeChunk(Chunk& chunk, ChunkType source, ChunkType destination)
{
  EN_ASSERT(source != destination, "Source and destination are the same!");
  EN_ASSERT(find(chunk.globalIndex()).first == source, "Chunk is not of the source type!");

  // Chunks moved from m_BoundaryChunks get queued for updating, along with all their neighbors
  if (source == ChunkType::Boundary)
    for (int i = -1; i <= 1; ++i)
      for (int j = -1; j <= 1; ++j)
        for (int k = -1; k <= 1; ++k)
        {
          GlobalIndex neighborIndex = chunk.globalIndex() + GlobalIndex(i, j, k);
          if (isLoaded(neighborIndex))
            m_LazyUpdateQueue.add(neighborIndex);
        }

  int sourceTypeID = static_cast<int>(source);
  int destinationTypeID = static_cast<int>(destination);

  m_Chunks[destinationTypeID].insert({ chunk.globalIndex(), &chunk});
  m_Chunks[sourceTypeID].erase(m_Chunks[sourceTypeID].find(chunk.globalIndex()));
}