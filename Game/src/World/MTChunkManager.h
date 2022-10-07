#pragma once
#include "Chunk.h"
#include <stack>
#include <shared_mutex>

class MTChunkManager
{
public:
  MTChunkManager();

  void render();

  void loadThread();
  void updateThread();

  void initializeChunks() {};
  bool loadNewChunks(int maxNewChunks) { return false; };
  bool updateChunks(int maxUpdates) { return false; };
  void clean() {};

private:
  static constexpr int s_RenderDistance = 16;
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

  // Mutexes
  mutable std::shared_mutex m_ChunkMapMutex;

  class IndexSet
  {
  public:
    bool add(const GlobalIndex& index);
    GlobalIndex waitAndRemoveOne();

  private:
    mapType<int, GlobalIndex> m_Data;
    std::mutex m_Mutex;
    std::condition_variable m_DataCondition;
  };

  IndexSet m_UpdateQueue;

// Helper functions for chunk container access. These assume map mutex has been locked somewhere up the call stack.
private:
  bool isLoaded(const GlobalIndex& chunkIndex) const;
  bool isOnBoundary(const Chunk* chunk) const;

  ChunkType getChunkType(const Chunk* chunk) const;
  Chunk* find(const GlobalIndex& chunkIndex);
  const Chunk* find(const GlobalIndex& chunkIndex) const;
  const Chunk* findNeighbor(const GlobalIndex& chunkIndex, Block::Face face) const;
  const Chunk* findNeighbor(const GlobalIndex& chunkIndex, Block::Face faceA, Block::Face faceB) const;
  const Chunk* findNeighbor(const GlobalIndex& chunkIndex, Block::Face faceA, Block::Face faceB, Block::Face faceC) const;

  void sendChunkLoadUpdate(Chunk* newChunk);
  void boundaryChunkUpdate(Chunk* chunk);
  void recategorizeChunk(Chunk* chunk, ChunkType source, ChunkType destination);
};