#include "GMpch.h"
#include "ChunkContainer.h"
#include "Player/Player.h"
#include "Util/Util.h"

ChunkContainer::ChunkContainer() = default;

const eng::threads::UnorderedMap<GlobalIndex, Chunk>& ChunkContainer::chunks() const
{
  return m_Chunks;
}

std::unordered_set<GlobalIndex> ChunkContainer::findAllLoadableIndices() const
{
  std::unordered_set<GlobalIndex> boundaryIndices = m_BoundaryIndices.getCurrentState();

  GlobalIndex originIndex = player::originIndex();
  std::unordered_set<GlobalIndex> newChunkIndices;
  for (const GlobalIndex& boundaryIndex : boundaryIndices)
    if (util::isInRange(boundaryIndex, originIndex, c_LoadDistance))
      newChunkIndices.insert(boundaryIndex);

  return newChunkIndices;
}

bool ChunkContainer::insert(const GlobalIndex& chunkIndex, const std::shared_ptr<Chunk>& newChunk)
{
  EN_ASSERT(newChunk, "Chunk does not exist!");

  bool chunkInserted = m_Chunks.insert(chunkIndex, newChunk);
  if (chunkInserted)
    boundaryUpdate(chunkIndex);

  return chunkInserted;
}

bool ChunkContainer::erase(const GlobalIndex& chunkIndex)
{
  bool chunkErased = m_Chunks.erase(chunkIndex);
  if (chunkErased)
    boundaryUpdate(chunkIndex);

  return chunkErased;
}

bool ChunkContainer::hasBoundaryNeighbors(const GlobalIndex& chunkIndex)
{
  return !Chunk::Stencil(chunkIndex).noneOf([this](const GlobalIndex& stencilIndex)
    {
      return m_BoundaryIndices.contains(stencilIndex);
    });
}



bool ChunkContainer::isOnBoundary(const GlobalIndex& chunkIndex) const
{
  for (eng::math::Direction direction : eng::math::Directions())
  {
    std::shared_ptr<Chunk> cardinalNeighbor = m_Chunks.get(chunkIndex + GlobalIndex::Dir(direction));
    if (cardinalNeighbor && !cardinalNeighbor->isFaceOpaque(!direction))
      return true;
  }
  return false;
}

void ChunkContainer::boundaryUpdate(const GlobalIndex& chunkIndex)
{
  Chunk::Stencil(chunkIndex).forEach([this](const GlobalIndex& neighborIndex)
    {
      if (m_Chunks.contains(neighborIndex) || !isOnBoundary(neighborIndex))
        m_BoundaryIndices.erase(neighborIndex);
      else
        m_BoundaryIndices.insert(neighborIndex);
    });
}
