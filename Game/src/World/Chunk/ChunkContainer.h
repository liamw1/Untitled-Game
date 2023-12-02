#pragma once
#include "Chunk.h"
#include "ChunkHelpers.h"

/*
  Class that handles the classification of chunks.
*/
class ChunkContainer
{
  eng::thread::UnorderedMap<GlobalIndex, Chunk> m_Chunks;
  eng::thread::UnorderedSet<GlobalIndex> m_BoundaryIndices;

public:
  ChunkContainer();

  const eng::thread::UnorderedMap<GlobalIndex, Chunk>& chunks() const;

  BlockArrayBox<block::Type> retrieveTypeData(const Chunk& chunk, const std::vector<BlockBox>& regions) const;
  BlockArrayBox<block::Light> retrieveLightingData(const Chunk& chunk, const std::vector<BlockBox>& regions) const;

  /*
    Scans boundary for places where new chunks can be loaded.

    \returns Possible locations a chunk can be loaded as an unordered set.
  */
  std::unordered_set<GlobalIndex> findAllLoadableIndices() const;

  /*
    Inserts chunk and adds it to boundary map. Its neighbors are moved from boundary map
    if they are no longer on the boundary.

    \returns True if the chunk was successfully inserted into the boundary map.
  */
  bool insert(const GlobalIndex& chunkIndex, const std::shared_ptr<Chunk>& newChunk);

  /*
    Removes chunk from boundary map, unloads it, and frees the slot it was occupying.
    The cardinal neighbors of the removed chunk are re-categorized as boundary chunks.

    \returns True if the chunk existed, was a boundary chunk, and was successfully removed.
  */
  bool erase(const GlobalIndex& chunkIndex);

  bool hasBoundaryNeighbors(const GlobalIndex& chunkIndex);

private:
  /*
    \returns True if the given chunk meets the requirements to be a boundary chunk.
             Does not check if the chunk is in the boundary map.
  */
  bool isOnBoundary(const GlobalIndex& chunkIndex) const;

  void boundaryUpdate(const GlobalIndex& chunkIndex);
};