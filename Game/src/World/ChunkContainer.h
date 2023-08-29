#pragma once
#include "Chunk.h"
#include <stack>
#include <shared_mutex>

// Will be removed eventually and replaced with settings system
constexpr int c_RenderDistance = 16;
constexpr int c_LoadDistance = c_RenderDistance + 1;
constexpr int c_UnloadDistance = c_LoadDistance;

/*
  Class that handles the classification of chunks.

  NOTE: With the changes to chunk rendering, its possible that ChunkType::Renderable
        is no longer needed. Implementation may be made simpler by removing that category.
*/
class ChunkContainer
{
public:
  ChunkContainer();

  /*
    Inserts chunk and adds it to boundary map. Its neighbors are moved from boundary map
    if they are no longer on the boundary.

    \returns True if the chunk was successfully inserted into the boundary map.
  */
  bool insert(Chunk&& newChunk);

  /*
    Removes chunk from boundary map, unloads it, and frees the slot it was occupying.
    The cardinal neighbors of the removed chunk are re-categorized as boundary chunks.

    \returns True if the chunk existed, was a boundary chunk, and was successfully removed.
  */
  bool erase(const GlobalIndex& chunkIndex);

  /*
    \returns All chunks of the specified type that match the given conditional.
  */
  template<typename F, typename... Args>
  std::vector<GlobalIndex> findAll(F condition, Args&&... args) const
  {
    std::vector<GlobalIndex> indexList;

    std::shared_lock lock(m_ContainerMutex);

    for (const auto& [key, chunk] : m_Chunks)
      if (condition(*chunk, std::forward<Args>(args)...))
        indexList.push_back(chunk->globalIndex());
    return indexList;
  }

  /*
    Scans boundary for places where new chunks can be loaded and returns possible locations
    as an unordered set.
  */
  std::unordered_set<GlobalIndex> findAllLoadableIndices() const;

  void uploadMeshes(Engine::Threads::UnorderedSet<Chunk::DrawCommand>& commandQueue, std::unique_ptr<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>& multiDrawArray) const;

  bool hasBoundaryNeighbors(const GlobalIndex& chunkIndex);

  /*
    \returns The chunk along with a lock on its mutex. Will return nullptr is no chunk is found.
  */
  [[nodiscard]] ChunkWithLock acquireChunk(const GlobalIndex& chunkIndex);
  [[nodiscard]] ConstChunkWithLock acquireChunk(const GlobalIndex& chunkIndex) const;

  bool empty() const;
  bool contains(const GlobalIndex& chunkIndex) const;

private:
  template<typename Key, typename Val>
  using mapType = std::unordered_map<Key, Val>;

  // Chunk pointers
  mapType<GlobalIndex, Chunk*> m_Chunks;
  std::unordered_set<GlobalIndex> m_BoundaryIndices;

  // Chunk data
  std::unique_ptr<Chunk[]> m_ChunkArray;
  std::stack<int, std::vector<int>> m_OpenChunkSlots;

  mutable std::shared_mutex m_ContainerMutex;

// Helper functions for chunk container access. These assume the map mutex has already been locked by one of the public functions
private:
  /*
    \returns True if the given chunk meets the requirements to be a boundary chunk.
             Does not check if the chunk is in the boundary map.

    Requires at minimum a shared lock to be owned on the container mutex.
  */
  bool isOnBoundary(const GlobalIndex& chunkIndex) const;

  /*
    \returns The Chunk and it's type at the specified chunk index.
             If no such chunk can be found, returns ChunkType::DNE and nullptr.

    Requires at minium a shared lock to be owned on the container mutex.
  */
  Chunk* find(const GlobalIndex& chunkIndex);
  const Chunk* find(const GlobalIndex& chunkIndex) const;

  void boundaryUpdate(const GlobalIndex& chunkIndex);
};