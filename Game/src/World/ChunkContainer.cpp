#include "GMpch.h"
#include "ChunkContainer.h"
#include "Player/Player.h"
#include "Indexing/Operations.h"

template<typename T>
static void fill(BlockArrayBox<T>& arrayBox, const Chunk& chunk, const std::vector<BlockBox>& chunkSections, const LocalIndex& relativeIndex)
{
  BlockIndex offset = Chunk::Size() * relativeIndex.checkedCast<blockIndex_t>();
  chunk.data<T>().readOperation([&arrayBox, &offset, &chunkSections](const BlockArrayBox<T>& chunkArrayBox, const T& defaultValue)
  {
    for (const BlockBox& chunkSection : chunkSections)
    {
      BlockBox fillSection = chunkSection + offset;
      arrayBox.fill(fillSection, chunkArrayBox, chunkSection, defaultValue);
    }
  });
}

template<typename T>
static BlockArrayBox<T> retrieveData(const Chunk& chunk, const std::vector<BlockBox>& regions, const eng::thread::UnorderedMap<GlobalIndex, Chunk>& chunkMap)
{
  BlockBox arrayBoxSize = eng::algo::accumulate(regions, eng::Identity<BlockBox>(), [](const BlockBox& boxSize, const BlockBox& box)
  {
    return boxSize.expandToEnclose(box);
  });
  BlockArrayBox<T> arrayBox(arrayBoxSize, eng::AllocationPolicy::DefaultInitialize);

  std::unordered_map<LocalIndex, std::vector<BlockBox>> partitionedRegions;
  for (const BlockBox& region : regions)
    for (const auto& [relativeIndex, partitionedRegion] : partitionBlockBox(region))
      partitionedRegions[relativeIndex].push_back(partitionedRegion);

  for (const auto& [relativeIndex, chunkSections] : partitionedRegions)
  {
    if (relativeIndex == LocalIndex(0))
      fill(arrayBox, chunk, chunkSections, relativeIndex);
    else if (std::shared_ptr<const Chunk> neighbor = chunkMap.get(chunk.globalIndex() + relativeIndex.upcast<globalIndex_t>()))
      fill(arrayBox, *neighbor, chunkSections, relativeIndex);
  }
  return arrayBox;
}



ChunkContainer::ChunkContainer() = default;

const eng::thread::UnorderedMap<GlobalIndex, Chunk>& ChunkContainer::chunks() const
{
  return m_Chunks;
}

BlockArrayBox<block::Type> ChunkContainer::retrieveTypeData(const Chunk& chunk, const std::vector<BlockBox>& regions) const
{
  return retrieveData<block::Type>(chunk, regions, m_Chunks);
}

BlockArrayBox<block::Light> ChunkContainer::retrieveLightingData(const Chunk& chunk, const std::vector<BlockBox>& regions) const
{
  return retrieveData<block::Light>(chunk, regions, m_Chunks);
}

std::unordered_set<GlobalIndex> ChunkContainer::findAllLoadableIndices() const
{
  std::unordered_set<GlobalIndex> boundaryIndices = m_BoundaryIndices.getCurrentState();

  GlobalIndex originIndex = player::originIndex();
  std::unordered_set<GlobalIndex> newChunkIndices;
  for (const GlobalIndex& boundaryIndex : boundaryIndices)
    if (isInRange(boundaryIndex, originIndex, param::LoadDistance()))
      newChunkIndices.insert(boundaryIndex);

  return newChunkIndices;
}

bool ChunkContainer::insert(const GlobalIndex& chunkIndex, const std::shared_ptr<Chunk>& newChunk)
{
  ENG_ASSERT(newChunk, "Chunk does not exist!");

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
  return eng::algo::anyOf(eng::math::Directions(), [this, &chunkIndex](eng::math::Direction direction)
  {
    std::shared_ptr<Chunk> cardinalNeighbor = m_Chunks.get(chunkIndex + GlobalIndex::Dir(direction));
    return cardinalNeighbor && !cardinalNeighbor->isFaceOpaque(!direction);
  });
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