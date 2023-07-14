#pragma once
#include "Indexing.h"
#include "Block/Block.h"
#include "Util/MultiDimArrays.h"

/*
  A class representing a NxNxN cube of blocks.

  Most functions require a lock on the chunk mutex to be used safely.
  There are two exceptions to this rule. The first is globalIndex(), which only needs a lock
  on the ChunkContainer mutex, as the global index of a chunk is only modified during insertion
  or deletion. The second is isFaceOpaque(face), which uses atomics for synchronization.
*/
class Chunk
{
private:
  static constexpr blockIndex_t c_ChunkSize = 32;

public:
  Chunk();
  Chunk(const GlobalIndex& chunkIndex);

  Chunk(Chunk&& other) noexcept;
  Chunk& operator=(Chunk&& other) noexcept;

  /*
    \returns The chunk's global index, which identifies it uniquely.
  */
  const GlobalIndex& globalIndex() const;

  bool empty() const;

  Block::Type getBlockType(const BlockIndex& blockIndex) const;
  Block::Type getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const;
  Block::Light getBlockLight(const BlockIndex& blockIndex) const;
  Block::Light getBlockLight(blockIndex_t i, blockIndex_t j, blockIndex_t k) const;

  /*
    \return Whether or not a given chunk face has transparent blocks. Useful for deciding which chunks should be loaded
    into memory.

    WARNING: This function only reflects the opacity state of the chunk when determineOpacity() was last called. If any
    changes to chunk composition occured since then, this function may not be accurate. This is because determineOpacity
    is a costly operation (at least relative to the cost of changing a single block), so we prefer to only compute opacity
    when the chunk is updated via setData or internalUpdate. Use with caution.
  */
  bool isFaceOpaque(Direction face) const;

  void setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType);
  void setBlockType(const BlockIndex& blockIndex, Block::Type blockType);
  void setBlockLight(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Light blockLight);
  void setBlockLight(const BlockIndex& blockIndex, Block::Light blockLight);

  void setComposition(Array3D<Block::Type, c_ChunkSize>&& composition);
  void setLighting(Array3D<Block::Light, c_ChunkSize>&& lighting);
  void determineOpacity();

  void update(bool hasMesh);
  void reset();

  /*
    \returns The chunk's geometric center relative to origin chunk.
  */
  static Vec3 Center(const Vec3& anchorPosition);

  /*
    A chunk's anchor point is its bottom southeast vertex.
    Position given relative to the anchor of the origin chunk.
    Useful property:
    If the anchor point is denoted by A, then for any point
    X within the chunk, X_i >= A_i.
  */
  static Vec3 AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex);

  static constexpr blockIndex_t Size() { return c_ChunkSize; }
  static constexpr length_t Length() { return Block::Length() * c_ChunkSize; }
  static constexpr int TotalBlocks() { return c_ChunkSize * c_ChunkSize * c_ChunkSize; }

  class Vertex
  {
  public:
    Vertex();
    Vertex(const BlockIndex& vertexPlacement, int quadIndex, int ambientOcclusion, Block::Texture texture);

    static const BlockIndex& GetOffset(Direction face, int quadIndex);

  private:
    uint32_t m_Data;
  };

  class Quad
  {
  public:
    Quad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, std::array<int, 4> ambientOcclusion);

  private:
    std::array<Vertex, 4> m_Vertices;
  };

  class Voxel
  {
  public:
    Voxel(const BlockIndex& blockIndex, uint8_t enabledFaces, int firstVertex);

    const BlockIndex& index() const;
    bool faceEnabled(Direction direction) const;
    int baseVertex() const;

  private:
    BlockIndex m_Index;
    uint8_t m_EnabledFaces;
    int m_BaseVertex;
  };

  class DrawCommand : public Engine::MultiDrawIndexedCommand<GlobalIndex, DrawCommand>
  {
  public:
    DrawCommand(const GlobalIndex& chunkIndex, bool canPruneIndices);

    DrawCommand(DrawCommand&& other) noexcept;
    DrawCommand& operator=(DrawCommand&& other) noexcept;

    bool operator==(const DrawCommand& other) const;

    int vertexCount() const;

    const void* indexData();
    const void* vertexData();
    void prune();

    void addQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, std::array<int, 4> ambientOcclusion);
    void addVoxel(const BlockIndex& blockIndex, uint8_t enabledFaces);

    void setIndices();
    bool sort(const GlobalIndex& originIndex, const Vec3& viewPosition);

  private:
    std::vector<Quad> m_Quads;
    std::vector<Voxel> m_Voxels;
    std::vector<uint32_t> m_Indices;
    BlockIndex m_SortState;
    bool m_CanPruneIndices;

    int m_VoxelBaseVertex;

    // Copy operators deleted to prevent copying of mesh data
    DrawCommand(const DrawCommand& other) = delete;
    DrawCommand& operator=(const DrawCommand& other) = delete;

    void addQuadIndices(int baseVertex);
    void setIndices(const GlobalIndex& originIndex, const Vec3& viewPosition);
  };

private:
  mutable std::mutex m_Mutex;
  Array3D<Block::Type, c_ChunkSize> m_Composition;
  Array3D<Block::Light, c_ChunkSize> m_Lighting;
  GlobalIndex m_GlobalIndex;
  std::atomic<uint16_t> m_NonOpaqueFaces;

  Chunk(const Chunk& other) = delete;
  Chunk& operator=(const Chunk& other) = delete;

  _Acquires_lock_(return) std::unique_lock<std::mutex> acquireLock() const;
  _Acquires_lock_(return) std::lock_guard<std::mutex> acquireLockGuard() const;

  friend class ChunkContainer;
};

namespace std
{
  template<>
  struct hash<Chunk::DrawCommand>
  {
    int operator()(const Chunk::DrawCommand& drawCommand) const
    {
      return std::hash<GlobalIndex>()(drawCommand.id());
    }
  };
}

struct ChunkWithLock
{
  Chunk* chunk;
  std::unique_lock<std::mutex> lock;
};

struct ConstChunkWithLock
{
  const Chunk* chunk;
  std::unique_lock<std::mutex> lock;
};