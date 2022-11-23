#include "GMpch.h"
#include "ChunkContainer.h"
#include "Util/Util.h"

static constexpr int s_MaxChunks = (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1);

ChunkContainer::ChunkContainer()
{
  m_ChunkArray = std::make_unique<Chunk[]>(s_MaxChunks);

  std::vector<int> stackData;
  stackData.reserve(s_MaxChunks);
  m_OpenChunkSlots = std::stack<int, std::vector<int>>(std::move(stackData));
  for (int i = s_MaxChunks - 1; i >= 0; --i)
    m_OpenChunkSlots.push(i);
}

bool ChunkContainer::insert(Chunk chunk)
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
  auto [insertionPosition, insertionSuccess] = m_BoundaryChunks.insert({ Util::CreateKey(newChunk->getGlobalIndex()), newChunk });

  if (insertionSuccess)
    sendChunkLoadUpdate(newChunk);

  return insertionSuccess;
}

bool ChunkContainer::erase(const GlobalIndex& chunkIndex)
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

bool ChunkContainer::update(const GlobalIndex& chunkIndex, const std::vector<uint32_t>& mesh)
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

  chunk->internalUpdate(mesh);

  ChunkType destination = chunk->empty() ? ChunkType::Empty : ChunkType::Renderable;
  if (source != destination)
  {
    std::lock_guard lock(m_ChunkMapMutex);
    recategorizeChunk(chunk, source, destination);
  }

  return true;
}

void ChunkContainer::forEach(ChunkType chunkType, const std::function<void(Chunk& chunk)>& func) const
{
  std::shared_lock lock(m_ChunkMapMutex);

  for (const auto& [key, chunk] : m_Chunks[static_cast<int>(chunkType)])
    func(*chunk);
}

std::vector<GlobalIndex> ChunkContainer::findAll(ChunkType chunkType, bool(*condition)(const Chunk& chunk)) const
{
  std::vector<GlobalIndex> indexList;

  std::shared_lock lock(m_ChunkMapMutex);

  for (const auto& [key, chunk] : m_Chunks[static_cast<int>(chunkType)])
    if (condition(*chunk))
      indexList.push_back(chunk->getGlobalIndex());
  return indexList;
}

std::unordered_set<GlobalIndex> ChunkContainer::findAllLoadableIndices() const
{
  std::shared_lock lock(m_ChunkMapMutex);

  std::unordered_set<GlobalIndex> newChunkIndices;
  for (const auto& [key, chunk] : m_BoundaryChunks)
    for (Block::Face face : Block::FaceIterator())
    {
      GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::Dir(face);
      if (Util::IsInRangeOfPlayer(neighborIndex, c_LoadDistance) && newChunkIndices.find(neighborIndex) == newChunkIndices.end())
        if (!chunk->isFaceOpaque(face) && !isLoaded(neighborIndex))
          newChunkIndices.insert(neighborIndex);
    }
  return newChunkIndices;
}

std::pair<Chunk*, std::unique_lock<std::mutex>> ChunkContainer::acquireChunk(const GlobalIndex& chunkIndex)
{
  std::shared_lock lock(m_ChunkMapMutex);
  std::unique_lock<std::mutex> chunkLock;

  Chunk* chunk = find(chunkIndex);
  if (chunk)
    chunkLock = std::unique_lock(chunk->m_Mutex);

  return { chunk, std::move(chunkLock) };
}

std::pair<const Chunk*, std::unique_lock<std::mutex>> ChunkContainer::acquireChunk(const GlobalIndex& chunkIndex) const
{
  std::shared_lock lock(m_ChunkMapMutex);
  std::unique_lock<std::mutex> chunkLock;

  const Chunk* chunk = find(chunkIndex);
  if (chunk)
    chunkLock = std::unique_lock(chunk->m_Mutex);

  return { chunk, std::move(chunkLock) };
}

void ChunkContainer::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
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
    GlobalIndex neighborIndex = chunkIndex + GlobalIndex::Dir(face);
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

bool ChunkContainer::empty() const
{
  std::shared_lock lock(m_ChunkMapMutex);
  return m_OpenChunkSlots.size() == s_MaxChunks;
}

bool ChunkContainer::contains(const GlobalIndex& chunkIndex) const
{
  std::shared_lock lock(m_ChunkMapMutex);
  return isLoaded(chunkIndex);
}



bool ChunkContainer::IndexSet::add(const GlobalIndex& index)
{
  std::lock_guard lock(m_Mutex);

  auto [insertionPosition, insertionSuccess] = m_Data.insert(index);
  return insertionSuccess;
}

std::optional<GlobalIndex> ChunkContainer::IndexSet::tryRemove()
{
  std::lock_guard lock(m_Mutex);

  if (!m_Data.empty())
  {
    GlobalIndex value = *m_Data.begin();
    m_Data.erase(m_Data.begin());
    return value;
  }
  return std::nullopt;
}



bool ChunkContainer::isLoaded(const GlobalIndex& chunkIndex) const
{
  for (const mapType<int, Chunk*>& chunkGroup : m_Chunks)
    if (chunkGroup.find(Util::CreateKey(chunkIndex)) != chunkGroup.end())
      return true;
  return false;
}

bool ChunkContainer::isOnBoundary(const Chunk* chunk) const
{
  EN_ASSERT(chunk, "Chunk does not exist!");

  for (Block::Face face : Block::FaceIterator())
    if (!isLoaded(chunk->getGlobalIndex() + GlobalIndex::Dir(face)) && !chunk->isFaceOpaque(face))
      return true;
  return false;
}

ChunkType ChunkContainer::getChunkType(const Chunk* chunk) const
{
  EN_ASSERT(chunk, "Chunk does not exist");

  for (int chunkType = 0; chunkType < s_ChunkTypes; ++chunkType)
    if (m_Chunks[chunkType].find(Util::CreateKey(chunk)) != m_Chunks[chunkType].end())
      return static_cast<ChunkType>(chunkType);

  EN_ERROR("Could not find chunk!");
  return ChunkType::Error;
}

Chunk* ChunkContainer::find(const GlobalIndex& chunkIndex)
{
  for (mapType<int, Chunk*>& chunkGroup : m_Chunks)
  {
    mapType<int, Chunk*>::iterator it = chunkGroup.find(Util::CreateKey(chunkIndex));

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

const Chunk* ChunkContainer::find(const GlobalIndex& chunkIndex) const
{
  for (const mapType<int, Chunk*>& chunkGroup : m_Chunks)
  {
    mapType<int, Chunk*>::const_iterator it = chunkGroup.find(Util::CreateKey(chunkIndex));

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

void ChunkContainer::sendChunkLoadUpdate(Chunk* newChunk)
{
  boundaryChunkUpdate(newChunk);

  // Move cardinal neighbors out of m_BoundaryChunks if they are no longer on boundary
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* neighbor = find(newChunk->getGlobalIndex() + GlobalIndex::Dir(face));
    if (neighbor && getChunkType(neighbor) == ChunkType::Boundary)
      boundaryChunkUpdate(neighbor);
  }
}

void ChunkContainer::sendChunkRemovalUpdate(const GlobalIndex& removalIndex)
{
  for (Block::Face face : Block::FaceIterator())
  {
    Chunk* neighbor = find(removalIndex + GlobalIndex::Dir(face));

    if (neighbor)
    {
      ChunkType type = getChunkType(neighbor);

      if (type != ChunkType::Boundary)
        recategorizeChunk(neighbor, type, ChunkType::Boundary);
    }
  }
}

void ChunkContainer::boundaryChunkUpdate(Chunk* chunk)
{
  EN_ASSERT(getChunkType(chunk) == ChunkType::Boundary, "Chunk is not a boundary chunk!");

  if (!isOnBoundary(chunk))
  {
    ChunkType destination = chunk->empty() ? ChunkType::Empty : ChunkType::Renderable;
    recategorizeChunk(chunk, ChunkType::Boundary, destination);
  }
}

void ChunkContainer::recategorizeChunk(Chunk* chunk, ChunkType source, ChunkType destination)
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