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

  bool isBlockNeighborAir(const Chunk* chunk, const BlockIndex& blockIndex, Block::Face face) const;
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
  using ChunkMapIterator = ChunkMap::iterator;

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
    Generates a (nearly) unique key for hash maps.
  */
  int createKey(const GlobalIndex& chunkIndex) const;
  int createHeightMapKey(globalIndex_t chunkI, globalIndex_t chunkJ) const;

  bool isInRange(const GlobalIndex& chunkIndex, globalIndex_t range) const;

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

  /*
    Searches for open chunk slot and loads chunk at the given index.
    New chunk is always categorized as a boundary chunk.

    \returns A valid iterator at the location of the chunk in m_BoundaryChunks.
  */
  ChunkMapIterator loadChunk(const GlobalIndex& chunkIndex);

  /*
    Searches for chunk, removes it from whatever map it is currently in,
    unloads it, and frees the slot it was occupying.  Only bondary chunks
    can be unloaded.

    The neighbors of the removed chunk are re-categorized as boundary chunks.

    \returns A valid iterator at the location of the next chunk in m_BoundaryChunks.
  */
  ChunkMapIterator unloadChunk(ChunkMapIterator erasePosition);

  void unloadUpdate(const GlobalIndex& chunkIndex);

  ChunkMapIterator updateChunk(ChunkMapIterator iteratorPosition);

  void fillChunk(Chunk* chunk, const HeightMap& heightMap);

  /*
    Generates simplistic mesh in a compressed format based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.

    Compresed format is follows,
     bits 0-17:  Relative position of vertex within chunk (3-comps, 6 bits each)
     bits 18-20: Normal direction of quad (follows BlockFace convention)
     bits 21-22: Texture coordinate index (see BlockFace.glsl for details)
     bits 23-24: Ambient Occlusion level
     bits 25-31: Texure ID

     Uses AO algorithm outlined in https ://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
  */
  void meshChunk(Chunk* chunk);

  Chunk* getNeighbor(Chunk* chunk, Block::Face face);
  Chunk* getNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB);
  Chunk* getNeighbor(Chunk* chunk, Block::Face faceA, Block::Face faceB, Block::Face faceC);
  const Chunk* getNeighbor(const Chunk* chunk, Block::Face face) const;

  bool isOnBoundary(const Chunk* chunk) const;

  bool isLoaded(const GlobalIndex& chunkIndex) const;

  Chunk* find(const GlobalIndex& chunkIndex);
  const Chunk* find(const GlobalIndex& chunkIndex) const;

  void addToGroup(Chunk* chunk, ChunkType destination);

  /*
    Moves chunk from one grouping to another.

    \param source      The type the chunk is currently classified as.
    \param destination The type the chunk will be classified as.

    \returns A valid iterator in source grouping.
  */
  ChunkMapIterator moveToGroup(ChunkMapIterator iteratorPosition, ChunkType source, ChunkType destination);
  void moveToGroup(Chunk* chunk, ChunkType source, ChunkType destination);

  ChunkType getChunkType(const Chunk* chunk);

  void queueForUpdating(Chunk* chunk);

  void updateImmediately(Chunk* chunk);

  void sendBlockUpdate(Chunk* chunk, const BlockIndex& blockIndex);
};