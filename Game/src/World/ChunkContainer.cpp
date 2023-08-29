#include "GMpch.h"
#include "ChunkContainer.h"
#include "Player/Player.h"
#include "Util/Util.h"

static constexpr int c_MaxChunks = (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1);

ChunkContainer::ChunkContainer() = default;

bool ChunkContainer::insert(Chunk&& newChunk)
{
  EN_PROFILE_FUNCTION();

  GlobalIndex chunkIndex = newChunk.globalIndex();
  bool insertionSuccess = m_Chunks.insert(chunkIndex, std::make_shared<Chunk>(std::move(newChunk)));

  if (insertionSuccess)
    boundaryUpdate2(chunkIndex);

  return insertionSuccess;
}

bool ChunkContainer::erase(const GlobalIndex& chunkIndex)
{
  m_Chunks.erase(chunkIndex);

  for (int i = -1; i <= 1; ++i)
    for (int j = -1; j <= 1; ++j)
      for (int k = -1; k <= 1; ++k)
        boundaryUpdate(chunkIndex + GlobalIndex(i, j, k));

  return true;
}

std::unordered_set<GlobalIndex> ChunkContainer::findAllLoadableIndices() const
{
  std::unordered_set<GlobalIndex> boundaryIndices = m_BoundaryIndices.getCurrentState();

  GlobalIndex originIndex = Player::OriginIndex();
  std::unordered_set<GlobalIndex> newChunkIndices;
  for (const GlobalIndex& boundaryIndex : boundaryIndices)
    if (Util::IsInRange(boundaryIndex, originIndex, c_LoadDistance))
      newChunkIndices.insert(boundaryIndex);

  return newChunkIndices;
}

void ChunkContainer::uploadMeshes(Engine::Threads::UnorderedSet<Chunk::DrawCommand>& commandQueue, std::unique_ptr<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>& multiDrawArray) const
{
  std::unordered_set<Chunk::DrawCommand> drawCommands = commandQueue.removeAll();

  std::vector<GlobalIndex> drawIDs;
  for (const Chunk::DrawCommand& drawCommand : drawCommands)
    drawIDs.push_back(drawCommand.id());

  std::unordered_map<GlobalIndex, std::shared_ptr<Chunk>> subset = m_Chunks.getSubsetOfCurrentState(drawIDs);
  while (!drawCommands.empty())
  {
    auto nodeHandle = drawCommands.extract(drawCommands.begin());
    Chunk::DrawCommand drawCommand = std::move(nodeHandle.value());

    multiDrawArray->remove(drawCommand.id());
    if (subset.contains(drawCommand.id()))
      multiDrawArray->add(std::move(drawCommand));
  }
}

bool ChunkContainer::hasBoundaryNeighbors(const GlobalIndex& chunkIndex)
{
  for (int i = -1; i <= 1; ++i)
    for (int j = -1; j <= 1; ++j)
      for (int k = -1; k <= 1; ++k)
        if (m_BoundaryIndices.contains(chunkIndex + GlobalIndex(i, j, k)))
          return true;
  return false;
}

ChunkWithLock ChunkContainer::acquireChunk(const GlobalIndex& chunkIndex) const
{
  std::unique_lock<std::mutex> chunkLock;

  std::shared_ptr<Chunk> chunk = find(chunkIndex);
  if (chunk)
    chunkLock = chunk->acquireLock();

  return { chunk, std::move(chunkLock) };
}

bool ChunkContainer::empty() const
{
  return m_Chunks.empty();
}

bool ChunkContainer::contains(const GlobalIndex& chunkIndex) const
{
  return m_Chunks.contains(chunkIndex);
}



bool ChunkContainer::isOnBoundary(const GlobalIndex& chunkIndex) const
{
  EN_ASSERT(!m_Chunks.contains(chunkIndex), "Given index is already loaded!");

  for (Direction direction : Directions())
  {
    std::shared_ptr<Chunk> cardinalNeighbor = find(chunkIndex + GlobalIndex::Dir(direction));
    if (cardinalNeighbor && !cardinalNeighbor->isFaceOpaque(!direction))
      return true;
  }
  return false;
}

std::shared_ptr<Chunk> ChunkContainer::find(const GlobalIndex& chunkIndex) const
{
  std::optional<std::shared_ptr<Chunk>> chunk = m_Chunks.get(chunkIndex);
  return chunk ? *chunk : nullptr;
}

void ChunkContainer::boundaryUpdate(const GlobalIndex& chunkIndex)
{
  if (m_Chunks.contains(chunkIndex) || !isOnBoundary(chunkIndex))
    m_BoundaryIndices.erase(chunkIndex);
  else
    m_BoundaryIndices.insert(chunkIndex);
}

void ChunkContainer::boundaryUpdate2(const GlobalIndex& chunkIndex)
{
  std::vector<GlobalIndex> surroundingIndices;
  for (int i = -2; i <= 2; ++i)
    for (int j = -2; j <= 2; ++j)
      for (int k = -2; k <= 2; ++k)
        surroundingIndices.push_back(chunkIndex + GlobalIndex(i, j, k));

  std::unordered_map<GlobalIndex, std::shared_ptr<Chunk>> chunks;
  chunks = m_Chunks.getSubsetOfCurrentState(surroundingIndices);

  auto isOnBoundary2 = [&chunks](const GlobalIndex& chunkIndex)
    {
      for (Direction direction : Directions())
      {
        auto cardinalNeighborPosition = chunks.find(chunkIndex + GlobalIndex::Dir(direction));
        if (cardinalNeighborPosition != chunks.end() && !cardinalNeighborPosition->second->isFaceOpaque(!direction))
          return true;
      }
      return false;
    };

  std::vector<GlobalIndex> indicesToErase;
  std::vector<GlobalIndex> indicesToInsert;
  for (int i = -1; i <= 1; ++i)
    for (int j = -1; j <= 1; ++j)
      for (int k = -1; k <= 1; ++k)
      {
        GlobalIndex neighborIndex = chunkIndex + GlobalIndex(i, j, k);

        if (chunks.contains(neighborIndex) || !isOnBoundary2(neighborIndex))
          indicesToErase.push_back(neighborIndex);
        else
          indicesToInsert.push_back(neighborIndex);
      }

  m_BoundaryIndices.eraseAll(indicesToErase);
  m_BoundaryIndices.insertAll(indicesToInsert);
}