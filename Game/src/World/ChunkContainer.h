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

  bool insert(Chunk chunk);
  bool erase(const GlobalIndex& chunkIndex);
  bool update(const GlobalIndex& chunkIndex, const std::vector<uint32_t>& mesh);

  void forEach(ChunkType chunkType, const std::function<void(Chunk& chunk)>& func) const;
  std::vector<GlobalIndex> findAll(ChunkType chunkType, bool (*condition)(const Chunk& chunk)) const;
  std::unordered_set<GlobalIndex> findAllLoadableIndices() const;

  [[nodiscard]] std::pair<Chunk*, std::unique_lock<std::mutex>> acquireChunk(const GlobalIndex& chunkIndex);
  [[nodiscard]] std::pair<const Chunk*, std::unique_lock<std::mutex>> acquireChunk(const GlobalIndex& chunkIndex) const;

  void sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  std::optional<GlobalIndex> getLazyUpdateIndex() { return m_LazyUpdateQueue.tryRemove(); }
  std::optional<GlobalIndex> getForceUpdateIndex() { return m_ForceUpdateQueue.tryRemove(); }

  bool empty() const;
  bool contains(const GlobalIndex& chunkIndex) const;

private:
  static constexpr int s_ChunkTypes = 3;

  template<typename Key, typename Val>
  using mapType = std::unordered_map<Key, Val>;

  // Chunk pointers
  std::array<mapType<int, Chunk*>, s_ChunkTypes> m_Chunks;
  mapType<int, Chunk*>& m_EmptyChunks = m_Chunks[static_cast<int>(ChunkType::Empty)];
  mapType<int, Chunk*>& m_BoundaryChunks = m_Chunks[static_cast<int>(ChunkType::Boundary)];
  mapType<int, Chunk*>& m_RenderableChunks = m_Chunks[static_cast<int>(ChunkType::Renderable)];

  // Chunk data
  std::unique_ptr<Chunk[]> m_ChunkArray;
  std::stack<int, std::vector<int>> m_OpenChunkSlots;

  mutable std::shared_mutex m_ChunkMapMutex;

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
  bool isLoaded(const GlobalIndex& chunkIndex) const;
  bool isOnBoundary(const Chunk* chunk) const;

  ChunkType getChunkType(const Chunk* chunk) const;
  Chunk* find(const GlobalIndex& chunkIndex);
  const Chunk* find(const GlobalIndex& chunkIndex) const;

  void sendChunkLoadUpdate(Chunk* newChunk);
  void sendChunkRemovalUpdate(const GlobalIndex& removalIndex);
  void boundaryChunkUpdate(Chunk* chunk);
  void recategorizeChunk(Chunk* chunk, ChunkType source, ChunkType destination);
};