#pragma once
#include "Chunk.h"
#include <llvm/ADT/DenseMap.h>

class ChunkManager
{
public:
  ChunkManager();

  void updatePlayerChunk(const ChunkIndex& playerChunkIndex) { m_PlayerChunkIndex = playerChunkIndex; };

  void clean();
  void render();
  bool loadNewChunks(uint32_t maxNewChunks);

private:
  enum class MapType : uint8_t
  {
    Boundary = 0,
    Empty,
    Renderable,

    First = Boundary, Last = Renderable
  };

  class MapTypeIterator
  {
  public:
    MapTypeIterator(const MapType mapType)
      : value(static_cast<uint8_t>(mapType)) {}
    MapTypeIterator()
      : value(static_cast<uint8_t>(MapType::First)) {}

    MapTypeIterator& operator++()
    {
      ++value;
      return *this;
    }
    MapType operator*() { return static_cast<MapType>(value); }
    bool operator!=(const MapTypeIterator& iter) { return value != iter.value; }

    MapTypeIterator begin() { return *this; }
    MapTypeIterator end()
    {
      static const MapTypeIterator endIter = ++MapTypeIterator(MapType::Last);
      return endIter;
    }

  private:
    uint8_t value;
  };

  static constexpr int s_RenderDistance = 16;
  static constexpr int s_LoadDistance = s_RenderDistance + 2;
  static constexpr int s_UnloadDistance = s_LoadDistance;
  static constexpr int s_TotalPossibleChunks = (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1);

  // Chunk data
  Engine::Unique<Chunk[]> m_ChunkArray;
  std::vector<int> m_OpenChunkSlots;

  // Chunk pointers
  std::array<llvm::DenseMap<int64_t, Chunk*>, 3> m_Chunks;
  llvm::DenseMap<int64_t, Chunk*>& m_EmptyChunks = m_Chunks[static_cast<uint8_t>(MapType::Empty)];
  llvm::DenseMap<int64_t, Chunk*>& m_BoundaryChunks = m_Chunks[static_cast<uint8_t>(MapType::Boundary)];
  llvm::DenseMap<int64_t, Chunk*>& m_RenderableChunks = m_Chunks[static_cast<uint8_t>(MapType::Renderable)];

  // Terrain data
  llvm::DenseMap<int64_t, HeightMap> m_HeightMaps{};

  // Player data
  ChunkIndex m_PlayerChunkIndex{};

  int64_t createKey(const ChunkIndex& chunkIndex) const;
  int64_t createHeightMapKey(int64_t chunkX, int64_t chunkY) const;

  bool isOutOfRange(const ChunkIndex& chunkIndex) const;
  bool isInLoadRange(const ChunkIndex& chunkIndex) const;
  bool isInRenderRange(const ChunkIndex& chunkIndex) const;

  HeightMap generateHeightMap(int64_t chunkX, int64_t chunkY);

  Chunk* loadNewChunk(const ChunkIndex& chunkIndex);
  void unloadChunk(Chunk* chunk);

  void addToMap(Chunk* chunk, MapType mapType);
  void moveToMap(Chunk* chunk, MapType source, MapType destination);
};