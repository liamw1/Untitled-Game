#pragma once
#include "Indexing.h"
#include "Block/Block.h"

/*
  A class representing a NxNxN cube of blocks.
*/
class Chunk : private Engine::NonCopyable, Engine::NonMovable
{
  static constexpr blockIndex_t c_ChunkSize = 32;

public:
  Chunk();
  Chunk(const GlobalIndex& chunkIndex);

  ArrayBox<Block::Type, 0, c_ChunkSize>& composition();
  const ArrayBox<Block::Type, 0, c_ChunkSize>& composition() const;

  ArrayBox<Block::Light, 0, c_ChunkSize>& lighting();
  const ArrayBox<Block::Light, 0, c_ChunkSize>& lighting() const;

  /*
    \return Whether or not a given chunk face has transparent blocks. Useful for deciding which chunks should be loaded
    into memory.

    WARNING: This function only reflects the opacity state of the chunk when determineOpacity() was last called. If any
    changes to chunk composition occured since then, this function may not be accurate. This is because determineOpacity
    is a costly operation (at least relative to the cost of changing a single block), so we prefer to only compute opacity
    when the chunk is updated via setData or internalUpdate. Use with caution.
  */
  bool isFaceOpaque(Direction face) const;

  Block::Type getBlockType(const BlockIndex& blockIndex) const;
  Block::Light getBlockLight(const BlockIndex& blockIndex) const;

  void setBlockType(const BlockIndex& blockIndex, Block::Type blockType);
  void setBlockLight(const BlockIndex& blockIndex, Block::Light blockLight);

  void setComposition(ArrayBox<Block::Type, 0, c_ChunkSize>&& composition);
  void setLighting(ArrayBox<Block::Light, 0, c_ChunkSize>&& lighting);
  void determineOpacity();

  void update();

  _Acquires_lock_(return) std::unique_lock<std::mutex> acquireLock() const;

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
  static constexpr length_t Length() { return Block::Length() * Size(); }
  static constexpr int TotalBlocks() { return Size() * Size() * Size(); }
  static constexpr BlockBox Bounds() { return BlockBox(0, Chunk::Size()); }
  static constexpr GlobalBox Stencil(const GlobalIndex& chunkIndex) { return GlobalBox(-1, 2) + chunkIndex; }

  class Vertex
  {
  public:
    Vertex();
    Vertex(const BlockIndex& vertexPlacement, int quadIndex, Block::Texture texture, int sunlight, int ambientOcclusion);

    static const BlockIndex& GetOffset(Direction face, int quadIndex);

  private:
    uint32_t m_VertexData;
    uint32_t m_Lighting;
  };

  class Quad
  {
  public:
    Quad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion);

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

  class DrawCommand : public Engine::MultiDrawIndexedCommand<GlobalIndex, DrawCommand>, private Engine::NonCopyable
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

    void addQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion);
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

    void addQuadIndices(int baseVertex);
    void setIndices(const GlobalIndex& originIndex, const Vec3& viewPosition);
  };

private:
  mutable std::mutex m_Mutex;
  ArrayBox<Block::Type, 0, c_ChunkSize> m_Composition;
  ArrayBox<Block::Light, 0, c_ChunkSize> m_Lighting;
  std::atomic<uint16_t> m_NonOpaqueFaces;
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
  std::shared_ptr<Chunk> chunk;
  std::unique_lock<std::mutex> lock;
};