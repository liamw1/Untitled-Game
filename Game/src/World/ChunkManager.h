#pragma once
#include "Chunk.h"
#include "ChunkContainer.h"
#include <stack>
#include <shared_mutex>

class ChunkManager
{
public:
  ChunkManager();
  ~ChunkManager();

  void render();
  void update();
  void clean();

  [[nodiscard]] std::pair<const Chunk*, std::unique_lock<std::mutex>> acquireChunk(const LocalIndex& chunkIndex) const;

  void placeBlock(const GlobalIndex& chunkIndex, BlockIndex blockIndex, Block::Face face, Block::Type blockType);
  void removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex);

  void setLoadModeTerrain() { m_LoadMode = LoadMode::Terrain; }
  void setLoadModeVoid() { m_LoadMode = LoadMode::Void; }
  void launchLoadThread() { m_LoadThread = std::thread(&ChunkManager::loadWorker, this); }
  void launchUpdateThread() { m_UpdateThread = std::thread(&ChunkManager::updateWorker, this); }

  // Debug
  void loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType);

private:
  enum LoadMode
  {
    NotSet = 0,
    Void,
    Terrain
  };

  // Multi-threading
  std::atomic<bool> m_Running;
  std::thread m_LoadThread;
  std::thread m_UpdateThread;
  ChunkContainer m_ChunkContainer;

  LoadMode m_LoadMode;
  GlobalIndex m_PrevPlayerOriginIndex;

  void loadWorker();
  void updateWorker();

  void generateNewChunk(const GlobalIndex& chunkIndex);
  std::vector<uint32_t> createMesh(const GlobalIndex& chunkIndex) const;
};