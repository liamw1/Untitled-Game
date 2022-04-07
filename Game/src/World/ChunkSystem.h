#pragma once
#include "NewChunk.h"

class ChunkManager
{
public:
  ChunkManager();
  ~ChunkManager();

  void initialize();

  /*
    Decides which chunks to submit for rendering.
  */
  void render() const;

  /*
    Loads new chunks inside load range.

    \returns True if at least one chunk has been loaded.
  */
  bool loadNewChunks(int maxNewChunks);

  /*
    Updates chunks in the update list.

    \returns True if at least one chunk has been updated.
  */
  bool updateChunks(int maxUpdates);

  /*
    Unloads chunks on boundary as they go out of unload range.
  */
  void clean();

  /*
    \returns The Chunk at the specified chunk index.
             If no such chunk can be found, returns nullptr.
  */
  Chunk* find(const LocalIndex& chunkIndex);
  const Chunk* find(const LocalIndex& chunkIndex) const;

  void placeBlock(Chunk* chunk, BlockIndex blockIndex, Block::Face face, Block::Type blockType);
  void removeBlock(Chunk* chunk, const BlockIndex& blockIndex);

private:
  enum class ChunkType : int
  {
    Boundary = 0,
    Empty,
    Renderable,
    NeedsUpdate,

    Error
  };

  enum class FrustumPlane : int
  {
    Left, Right,
    Bottom, Top,
    Near, Far
  };

private:
  using ChunkMap = std::unordered_map<int, Chunk*>;

  static constexpr int s_RenderDistance = 16;
  static constexpr int s_LoadDistance = s_RenderDistance + 2;
  static constexpr int s_UnloadDistance = s_LoadDistance;
  static constexpr int s_MaxChunks = (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1);

  // Chunk data
  Chunk* m_ChunkArray;
  std::vector<int> m_OpenChunkSlots;

  // Chunk pointers
  std::array<ChunkMap, 4> m_Chunks;
  ChunkMap& m_EmptyChunks = m_Chunks[static_cast<int>(ChunkType::Empty)];
  ChunkMap& m_BoundaryChunks = m_Chunks[static_cast<int>(ChunkType::Boundary)];
  ChunkMap& m_RenderableChunks = m_Chunks[static_cast<int>(ChunkType::Renderable)];
  ChunkMap& m_UpdateList = m_Chunks[static_cast<int>(ChunkType::NeedsUpdate)];
  std::unordered_map<int, HeightMap> m_HeightMaps;

  int m_ChunksLoaded = 0;

  /*
    Grabs first open chunk slot and loads chunk at the given index.
    New chunk is always categorized as a boundary chunk to start with.

    \returns A valid iterator at the location of the chunk in m_BoundaryChunks.
  */
  ChunkMap::iterator loadChunk(const GlobalIndex& chunkIndex);

  /*
    Gives chunk necessary updates and removes it from the update list.
    Given chunk must be in the update list.

    \returns A valid iterator pointing to the next chunk in m_UpdateList.
  */
  ChunkMap::iterator updateChunk(ChunkMap::iterator iteratorPosition);

  /*
    Removes chunk from map, unloads it, and frees the slot it was occupying.
    Given chunk must be a boundary chunk.

    The cardinal neighbors of the removed chunk are re-categorized as boundary chunks.

    \returns A valid iterator pointing to the next chunk in m_BoundaryChunks.
  */
  ChunkMap::iterator unloadChunk(ChunkMap::iterator erasePosition);

  /*
    Re-categorizes the cardinal neighbors of a removed chunk as boundary chunks.
    Should be called for each chunk removed by unloadChunk function.
  */
  void sendChunkRemovalUpdate(const GlobalIndex& chunkIndex);

  void queueForUpdating(Chunk* chunk);
  void updateImmediately(Chunk* chunk);
  void sendBlockUpdate(Chunk* chunk, const BlockIndex& blockIndex);

  /*
    Uses heightmap to fill a chunk with block data.
  */
  void fillChunk(Chunk* chunk, const HeightMap& heightMap);

  /*
    Generates simplistic mesh in a compressed format based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.

    Compresed format is follows,
     bits 0-17:  Relative position of vertex within chunk (3-comps, 6 bits each)
     bits 18-19: Texture coordinate index (see Chunk.glsl for details)
     bits 20-21: Ambient Occlusion level (see link below)
     bits 22-31: Texure ID

     Uses AO algorithm outlined in https ://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
  */
  void meshChunk(Chunk* chunk);

  Chunk* find(const GlobalIndex& chunkIndex);
  const Chunk* find(const GlobalIndex& chunkIndex) const;

  Chunk* findNeighbor(Chunk* chunk, Block::Face face);
  Chunk* findNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB);
  Chunk* findNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB, Block::Face faceC);
  const Chunk* findNeighbor(const Chunk* chunk, Block::Face face) const;

  bool isOnBoundary(const Chunk* chunk) const;
  bool isLoaded(const GlobalIndex& chunkIndex) const;
  ChunkType getChunkType(const Chunk* chunk);

  void addToGroup(Chunk* chunk, ChunkType destination);

  /*
    Moves chunk from one grouping to another.

    \param source      The type the chunk is currently classified as.
    \param destination The type the chunk will be classified as.

    \returns A valid iterator in source grouping.
  */
  ChunkMap::iterator moveToGroup(ChunkMap::iterator iteratorPosition, ChunkType source, ChunkType destination);
  void moveToGroup(Chunk* chunk, ChunkType source, ChunkType destination);

  bool blockNeighborIsAir(const Chunk* chunk, const BlockIndex& blockIndex, Block::Face face) const;

  /*
    Generates a (nearly) unique key for hash maps.
  */
  int createKey(const GlobalIndex& chunkIndex) const;
  int createHeightMapKey(globalIndex_t chunkI, globalIndex_t chunkJ) const;

  bool isInRange(const GlobalIndex& chunkIndex, globalIndex_t range) const;

  // NOTE: These should probably be moved out into Utils folder
  /*
  Uses algorithm described in
  https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

  \returns An array of vectors representing the view frustum planes.
           For a plane of the form Ax + By + Cz + D = 0,
           its corresponding vector is {A, B, C, D}.
  */
  std::array<Vec4, 6> calculateViewFrustumPlanes(const Mat4& viewProjection) const;

  /*
    \returns True if the given point is inside the given set of frustum planes.
             Could be any frustum, not necessarily the view frustum.
  */
  bool isInFrustum(const Vec3& point, const std::array<Vec4, 6>& frustumPlanes) const;
};