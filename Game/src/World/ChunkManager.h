#pragma once
#include "Chunk.h"
#include <llvm/ADT/DenseMap.h>

class ChunkManager
{
public:
  ChunkManager();

  /*
    Unloads chunks on boundary as they go out of unload range.
  */
  void clean();

  /*
    Decides which chunks to submit for rendering.
  */
  void render(const Engine::Camera& playerCamera);

  /*
    Loads new chunks inside load range.

    \returns True if at least one chunk has been loaded.
  */
  bool loadNewChunks(uint32_t maxNewChunks);

  /*
    For updating the chunk manager of what chunk the player is currently inside.
  */
  void updatePlayerChunk(const ChunkIndex& playerChunkIndex) { m_PlayerChunkIndex = playerChunkIndex; };

private:
  enum class MapType : uint8_t
  {
    Boundary = 0,
    Empty,
    Renderable,

    First = Boundary, Last = Renderable
  };
  using MapTypeIterator = Iterator<MapType, MapType::First, MapType::Last>;

  enum class FrustumPlane : uint8_t
  {
    Left, Right,
    Bottom, Top,
    Near, Far
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

  /*
    Generates a (nearly) unique key for chunk hash maps.
  */
  int64_t createKey(const ChunkIndex& chunkIndex) const;
  int64_t createHeightMapKey(int64_t chunkI, int64_t chunkJ) const;

  bool isOutOfRange(const ChunkIndex& chunkIndex) const;
  bool isInLoadRange(const ChunkIndex& chunkIndex) const;
  bool isInRenderRange(const ChunkIndex& chunkIndex) const;

  /*
    Uses algorithm described in 
    https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    
    \returns An array of vectors representing the view frustum planes.
             For a plane of the form Ax + By + Cz + D = 0, 
             its corresponding vector is {A, B, C, D}.
  */
  std::array<glm::vec4, 6> calculateViewFrustumPlanes(const Engine::Camera& playerCamera) const;

  /*
    \returns True if the given point is inside the given set of frustum planes.
             Could be any frustum, not necessarily the view frustum.
  */
  bool isInFrustum(const glm::vec3& point, const std::array<glm::vec4, 6>& frustumPlanes) const;

  /*
    Generates a chunk-sized section of terrain heights using simplex noise.
  */
  HeightMap generateHeightMap(int64_t chunkI, int64_t chunkJ);

  /*
    Searches for open chunk slot and loads chunk at the given index.
    New chunk is always categorized as a boundary chunk.

    \returns A pointer to the newly created chunk.
  */
  Chunk* loadNewChunk(const ChunkIndex& chunkIndex);

  /*
    Searches for chunk, removes it from whatever map it is currently in,
    unloads it, and frees the slot it was occupying.  Only bondary chunks
    can be unloaded.

    The neighbors of the removed chunk are re-categorized as boundary chunks.
  */
  void unloadChunk(Chunk* chunk);

  /*
    Adds chunk pointer to the specified map.
    Chunk should be in any other map before calling this function.
  */
  void addToMap(Chunk* chunk, MapType mapType);

  /*
    Moves chunk pointer from one map to another.

    \param source      The map the chunk is currently inside.
    \param destination The map the chunk will placed into.
  */
  void moveToMap(Chunk* chunk, MapType source, MapType destination);
};