#pragma once
#include "Chunk.h"
#include "LOD.h"

class ChunkManager
{
public:
  ChunkManager();
  ~ChunkManager();

  void initialize();

  /*
    Unloads chunks on boundary as they go out of unload range.
  */
  void clean();

  /*
    Decides which chunks to submit for rendering.
  */
  void render() const;

  /*
    Loads new chunks inside load range.

    \returns True if at least one chunk has been loaded.
  */
  bool loadNewChunks(int maxNewChunks);

  void renderLODs();
  void manageLODs();

  /*
    \returns The Chunk at the specified chunk index.  If no such chunk can be found,
             returns nullptr.
  */
  Chunk* findChunk(const LocalIndex& chunkIndex) const;

  /*
    Sends message to chunk manager that chunk has been modified.
    IMPORTANT: Should be called after chunk modifications.
  */
  void sendChunkUpdate(Chunk* const chunk);

private:
  enum class MapType : int
  {
    Boundary = 0,
    Empty,
    Renderable,
    
    First = Boundary, Last = Renderable
  };
  using MapTypeIterator = Iterator<MapType, MapType::First, MapType::Last>;

  enum class FrustumPlane : int
  {
    Left, Right,
    Bottom, Top,
    Near, Far
  };

  struct LODVertex
  {
    Vec3 position;
    int quadIndex;
    float lightValue;
  };

  static constexpr int s_RenderDistance = 4;
  static constexpr int s_LoadDistance = s_RenderDistance + 2;
  static constexpr int s_UnloadDistance = s_LoadDistance;
  static constexpr int s_MaxChunks = (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1) * (2 * s_UnloadDistance + 1);

  // Chunk data
  Chunk* m_ChunkArray;
  std::vector<int> m_OpenChunkSlots;

  // Chunk pointers
  std::array<llvm::DenseMap<int, Chunk*>, 3> m_Chunks;
  llvm::DenseMap<int, Chunk*>& m_EmptyChunks = m_Chunks[static_cast<int>(MapType::Empty)];
  llvm::DenseMap<int, Chunk*>& m_BoundaryChunks = m_Chunks[static_cast<int>(MapType::Boundary)];
  llvm::DenseMap<int, Chunk*>& m_RenderableChunks = m_Chunks[static_cast<int>(MapType::Renderable)];
  // NOTE: Should switch to more cache-friendly map, such as tsl::sparse_map

  // LOD data
  LOD::Octree m_LODTree{};

  // Terrain data
  llvm::DenseMap<int, HeightMap> m_HeightMaps{};

  /*
    Generates a (nearly) unique key for chunk hash maps.
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
    Generates a chunk-sized section of terrain heights using simplex noise.
  */
  HeightMap generateHeightMap(globalIndex_t chunkI, globalIndex_t chunkJ);

  /*
    Searches for open chunk slot and loads chunk at the given index.
    New chunk is always categorized as a boundary chunk.

    \returns A pointer to the newly created chunk.
  */
  Chunk* loadChunk(const GlobalIndex& chunkIndex);

  /*
    Searches for chunk, removes it from whatever map it is currently in,
    unloads it, and frees the slot it was occupying.  Only bondary chunks
    can be unloaded.

    The neighbors of the removed chunk are re-categorized as boundary chunks.
  */
  void unloadChunk(Chunk* const chunk);

  /*
    \returns The Chunk at the specified chunk index.  If no such chunk can be found,
             returns nullptr.
  */
  Chunk* findChunk(const GlobalIndex& chunkIndex) const;

  /*
    Adds chunk pointer to the specified map.
    Chunk should be in any other map before calling this function.
  */
  void addToMap(Chunk* const chunk, MapType mapType);

  /*
    Moves chunk pointer from one map to another.

    \param source      The map the chunk is currently inside.
    \param destination The map the chunk will placed into.
  */
  void moveToMap(Chunk* const chunk, MapType source, MapType destination);

  bool isOnBoundary(const Chunk* const chunk) const;

  void initializeLODs();
  bool splitLODs(std::vector<LOD::Octree::Node*>& leaves);
  bool combineLODs(std::vector<LOD::Octree::Node*>& leaves);
  bool splitAndCombineLODs(std::vector<LOD::Octree::Node*>& leaves);
};