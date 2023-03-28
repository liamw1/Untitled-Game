#pragma once
#include "Chunk.h"
#include <stack>
#include <shared_mutex>

// Will be removed eventually and replaced with settings system
constexpr int c_RenderDistance = 16;
constexpr int c_LoadDistance = c_RenderDistance + 2;
constexpr int c_UnloadDistance = c_LoadDistance;

enum class ChunkType
{
  Boundary = 0,
  Empty,
  Renderable,

  Error
};

class ChunkContainer
{
public:
  ChunkContainer();

  /*
    Inserts chunk and adds it to boundary map. Its neighbors are moved from boundary map
    if they are no longer on the boundary.

    \returns True if the chunk was successfully inserted into the boundary map.
  */
  bool insert(Chunk&& chunk);

  /*
    Removes chunk from boundary map, unloads it, and frees the slot it was occupying.
    The cardinal neighbors of the removed chunk are re-categorized as boundary chunks.

    \returns True if the chunk existed, was a boundary chunk, and was successfully removed.
  */
  bool erase(const GlobalIndex& chunkIndex);

  /*
    Updates the chunk at the given index.
    Boundary chunks will not receive updates.

    \returns True if the chunk existed, was not a boundary chunk, and was successfully updated.
  */
  bool update(const GlobalIndex& chunkIndex, const std::vector<uint32_t>& mesh);

  /*
    Used to operate on each chunk of the specified type.
  */
  void forEach(ChunkType chunkType, const std::function<void(Chunk& chunk)>& func) const;

  /*
    \returns All chunks of the specified type that match the given conditional.
  */
  std::vector<GlobalIndex> findAll(ChunkType chunkType, bool (*condition)(const Chunk& chunk)) const;

  /*
    Scans boundary for places where new chunks can be loaded and returns possible locations
    as an unordered set.
  */
  std::unordered_set<GlobalIndex> findAllLoadableIndices() const;

  /*
    \returns The chunk along with a lock on its mutex. Will return nullptr is no chunk is found.
  */
  [[nodiscard]] std::pair<Chunk*, std::unique_lock<std::mutex>> acquireChunk(const GlobalIndex& chunkIndex);
  [[nodiscard]] std::pair<const Chunk*, std::unique_lock<std::mutex>> acquireChunk(const GlobalIndex& chunkIndex) const;

  /*
    Queues chunk where the block update occured for updating. If specified block is on chunk border,
    will also update neighboring chunks. Chunk and its cardinal neighbors are queue for an immediate update,
    while edge and corner neighbors are queued for later.
  */
  void sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  std::optional<GlobalIndex> getLazyUpdateIndex() { return m_LazyUpdateQueue.tryRemove(); }
  std::optional<GlobalIndex> getForceUpdateIndex() { return m_ForceUpdateQueue.tryRemove(); }

  bool empty() const;
  bool contains(const GlobalIndex& chunkIndex) const;

private:
  static constexpr int c_ChunkTypes = 3;

  template<typename Key, typename Val>
  using mapType = std::unordered_map<Key, Val>;

  // Chunk pointers
  std::array<mapType<int, Chunk*>, c_ChunkTypes> m_Chunks;
  mapType<int, Chunk*>& m_EmptyChunks = m_Chunks[static_cast<int>(ChunkType::Empty)];
  mapType<int, Chunk*>& m_BoundaryChunks = m_Chunks[static_cast<int>(ChunkType::Boundary)];
  mapType<int, Chunk*>& m_RenderableChunks = m_Chunks[static_cast<int>(ChunkType::Renderable)];

  // Chunk data
  std::unique_ptr<Chunk[]> m_ChunkArray;
  std::stack<int, std::vector<int>> m_OpenChunkSlots;

  mutable std::shared_mutex m_ContainerMutex;

  class IndexSet
  {
  public:
    bool add(const GlobalIndex& index);
    std::optional<GlobalIndex> tryRemove();

  private:
    std::unordered_set<GlobalIndex> m_Data;
    std::mutex m_Mutex;
  };

  IndexSet m_LazyUpdateQueue;
  IndexSet m_ForceUpdateQueue;

// Helper functions for chunk container access. These assume the map mutex has already been locked by one of the public functions
private:
  /*
    \returns True if there exists a chunk in the container at the specified index.

    Requires at minimum a shared lock to be owned on the container mutex.
  */
  bool isLoaded(const GlobalIndex& chunkIndex) const;

  /*
    \returns True if the given chunk meets the requirements to be a boundary chunk.
             Does not check if the chunk is in the boundary map.

    Requires at minimum a shared lock to be owned on the container mutex.
  */
  bool isOnBoundary(const Chunk& chunk) const;

  /*
    \returns What type the chunk is currently classified as.

    Requires at minimum a shared lock to be owned on the container mutex.
  */
  ChunkType getChunkType(const Chunk& chunk) const;

  /*
    \returns The Chunk at the specified chunk index. If no such chunk can be found, returns nullptr.

    Requires at minium a shared lock to be owned on the container mutex.
  */
  Chunk* find(const GlobalIndex& chunkIndex);
  const Chunk* find(const GlobalIndex& chunkIndex) const;

  /*
    Re-categorizes loaded chunk and its cardinal neighbors if they are no longer on boundary.
    Should be called for each newly-loaded chunk.

    Requires an exclusive lock on the container mutex, as it will modify chunk maps.
  */
  void sendChunkLoadUpdate(Chunk& newChunk);

  /*
    Re-categorizes the cardinal neighbors of a removed chunk as boundary chunks.
    Should be called for each chunk removed by unloadChunk function.

    Requires an exclusive lock on the container mutex, as it will modify chunk maps.
  */
  void sendChunkRemovalUpdate(const GlobalIndex& removalIndex);

  /*
    Moves chunk from m_BoundaryChunks if no longer on boundary.
    Given chunk must be a boundary chunk.

    Requires an exclusive lock on the container mutex, as it will modify chunk maps.
  */
  void boundaryChunkUpdate(Chunk& chunk);

  /*
    Moves chunk from one grouping to another.
    \param source      The type the chunk is currently classified as.
    \param destination The type the chunk will be classified as.

    Requires an exclusive lock on the container mutex, as it will modify chunk maps.
  */
  void recategorizeChunk(Chunk& chunk, ChunkType source, ChunkType destination);
};