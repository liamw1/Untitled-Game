#pragma once
#include "Chunk.h"
#include <stack>
#include <shared_mutex>

class MTChunkManager
{
public:
  MTChunkManager();
  ~MTChunkManager();

  void render();

  void loadWorker();
  void updateWorker();

  bool loadNewChunks(int maxNewChunks) { return false; };
  void updateChunks(int maxUpdates);
  void clean();

  [[nodiscard]] std::pair<const Chunk*, std::unique_lock<std::mutex>> acquireChunk(const LocalIndex& chunkIndex) const;

  void placeBlock(const GlobalIndex& chunkIndex, BlockIndex blockIndex, Block::Face face, Block::Type blockType);
  void removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

private:
  static constexpr int s_RenderDistance = 32;
  static constexpr int s_LoadDistance = s_RenderDistance + 2;
  static constexpr int s_UnloadDistance = s_LoadDistance;
  static constexpr int s_MaxChunks = (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1);

  enum class ChunkType : int
  {
    Boundary = 0,
    Empty,
    Renderable,

    Error
  };

// Functions which control access to chunk containers
private:
  bool insert(Chunk&& chunk);
  bool erase(const GlobalIndex& chunkIndex);
  bool update(const GlobalIndex& chunkIndex, const std::vector<uint32_t>& mesh);

  std::vector<uint32_t> createMesh(const GlobalIndex& chunkIndex) const;

  void forEach(ChunkType chunkType, const std::function<void(Chunk& chunk)>& func) const;
  std::vector<GlobalIndex> findAll(ChunkType chunkType, bool (*condition)(const Chunk& chunk)) const;

  [[nodiscard]] std::pair<Chunk*, std::unique_lock<std::mutex>> acquireChunk(const GlobalIndex& chunkIndex);
  [[nodiscard]] std::pair<const Chunk*, std::unique_lock<std::mutex>> acquireChunk(const GlobalIndex& chunkIndex) const;

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
  Chunk* m_ChunkArray;
  std::stack<int, std::vector<int>> m_OpenChunkSlots;

  // Multi-threading
  std::atomic<bool> m_Running;
  std::thread m_LoadThread;
  std::thread m_UpdateThread;
  mutable std::shared_mutex m_ChunkMapMutex;

  class IndexSet
  {
  public:
    bool add(const GlobalIndex& index);
    GlobalIndex waitAndRemoveOne();
    std::optional<GlobalIndex> tryRemove();

  private:
    mapType<int, GlobalIndex> m_Data;
    std::mutex m_Mutex;
    std::condition_variable m_DataCondition;
  };

  IndexSet m_LazyUpdateQueue;
  IndexSet m_ForceUpdateQueue;

// Helper functions for chunk container access. These assume map mutex has been locked somewhere up the call stack.
private:
  bool isLoaded(const GlobalIndex& chunkIndex) const;
  bool isOnBoundary(const Chunk* chunk) const;

  ChunkType getChunkType(const Chunk* chunk) const;
  Chunk* find(const GlobalIndex& chunkIndex);
  const Chunk* find(const GlobalIndex& chunkIndex) const;

  void sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);
  void sendChunkLoadUpdate(Chunk* newChunk);
  void sendChunkRemovalUpdate(const GlobalIndex& removalIndex);
  void boundaryChunkUpdate(Chunk* chunk);
  void recategorizeChunk(Chunk* chunk, ChunkType source, ChunkType destination);
};