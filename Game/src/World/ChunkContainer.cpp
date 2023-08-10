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

bool ChunkContainer::canMesh(const GlobalIndex& chunkIndex)
{
  EN_PROFILE_FUNCTION();

  std::shared_lock lock(m_ContainerMutex);

  for (int i = -1; i <= 1; ++i)
    for (int j = -1; j <= 1; ++j)
      for (int k = -1; k <= 1; ++k)
        if (m_BoundaryIndices.contains(chunkIndex + GlobalIndex(i, j, k)))
          return false;
  return true;
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
  auto [insertionPosition, insertionSuccess] = m_Chunks.insert({ chunk.globalIndex(), &chunk });

  if (insertionSuccess)
    for (int i = -1; i <= 1; ++i)
      for (int j = -1; j <= 1; ++j)
        for (int k = -1; k <= 1; ++k)
        {
          GlobalIndex neighborIndex = chunk.globalIndex() + GlobalIndex(i, j, k);
          boundaryUpdate(neighborIndex);
          m_LazyUpdateQueue.add(neighborIndex);
        }

  return insertionSuccess;
}

bool ChunkContainer::erase(const GlobalIndex& chunkIndex)
{
  std::lock_guard lock(m_ContainerMutex);

  mapType<GlobalIndex, Chunk*>::iterator erasePosition = m_Chunks.find(chunkIndex);
  if (erasePosition == m_Chunks.end())
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
  m_Chunks.erase(erasePosition);

  for (int i = -1; i <= 1; ++i)
    for (int j = -1; j <= 1; ++j)
      for (int k = -1; k <= 1; ++k)
        boundaryUpdate(chunkIndex + GlobalIndex(i, j, k));

  return true;
}

bool ChunkContainer::update(const GlobalIndex& chunkIndex, bool meshGenerated)
{
  auto [chunk, lock] = acquireChunk(chunkIndex);

  if (!chunk)
    return false;

  chunk->update(meshGenerated);

  return true;
}

std::unordered_set<GlobalIndex> ChunkContainer::findAllLoadableIndices() const
{
  std::shared_lock lock(m_ContainerMutex);

  GlobalIndex originIndex = Player::OriginIndex();
  std::unordered_set<GlobalIndex> newChunkIndices;
  for (const GlobalIndex& boundaryIndex : m_BoundaryIndices)
    if (Util::IsInRange(boundaryIndex, originIndex, c_LoadDistance))
      newChunkIndices.insert(boundaryIndex);

  return newChunkIndices;
}

void ChunkContainer::uploadMeshes(Engine::Threads::UnorderedSetQueue<Chunk::DrawCommand>& commandQueue, std::unique_ptr<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>& multiDrawArray) const
{
  std::shared_lock lock(m_ContainerMutex);

  std::optional<Chunk::DrawCommand> drawCommand = commandQueue.tryRemove();
  while (drawCommand)
  {
    multiDrawArray->remove(drawCommand->id());
    if (find(drawCommand->id()))
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

  const Chunk* chunk = find(chunkIndex);
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
  return find(chunkIndex);
}



bool ChunkContainer::isOnBoundary(const GlobalIndex& chunkIndex) const
{
  EN_ASSERT(!m_Chunks.contains(chunkIndex), "Given index is already loaded!");

  for (Direction direction : Directions())
  {
    const Chunk* cardinalNeighbor = find(chunkIndex + GlobalIndex::Dir(direction));
    if (cardinalNeighbor && !cardinalNeighbor->isFaceOpaque(!direction))
      return true;
  }
  return false;
}

Chunk* ChunkContainer::find(const GlobalIndex& chunkIndex)
{
  return const_cast<Chunk*>(static_cast<const ChunkContainer*>(this)->find(chunkIndex));
}

const Chunk* ChunkContainer::find(const GlobalIndex& chunkIndex) const
{
  mapType<GlobalIndex, Chunk*>::const_iterator it = m_Chunks.find(chunkIndex);
  return it == m_Chunks.end() ? nullptr : it->second;
}

void ChunkContainer::boundaryUpdate(const GlobalIndex& chunkIndex)
{
  if (m_Chunks.contains(chunkIndex) || !isOnBoundary(chunkIndex))
    m_BoundaryIndices.erase(chunkIndex);
  else
    m_BoundaryIndices.insert(chunkIndex);
}