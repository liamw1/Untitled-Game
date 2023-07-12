#pragma once
#include "Indexing.h"
#include "Block/Block.h"
#include "Util/MultiDimArrays.h"

/*
  A class representing a NxNxN cube of blocks.
*/
class Chunk
{
/*
  Most public functions can be safely accessed as long as a lock is held on either
  the chunk's mutex OR the container mutex. The only exceptions are empty(), and
  the getBlockType() variants, which require a lock on the chunk mutex specifically.
*/
public:
  Chunk();
  Chunk(const GlobalIndex& chunkIndex);

  Chunk(Chunk&& other) noexcept;
  Chunk& operator=(Chunk&& other) noexcept;

  // Chunks are unique and so cannot be copied
  Chunk(const Chunk& other) = delete;
  Chunk& operator=(const Chunk& other) = delete;

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

  static constexpr blockIndex_t Size() { return c_ChunkSize; }
  static constexpr length_t Length() { return Block::Length() * c_ChunkSize; }
  static constexpr int TotalBlocks() { return c_ChunkSize * c_ChunkSize * c_ChunkSize; }

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

// All private functions require a lock on the chunk mutex.
private:
  static constexpr blockIndex_t c_ChunkSize = 32;

  mutable std::mutex m_Mutex;
  Array3D<Block::Type, c_ChunkSize> m_Composition;
  Array3D<Block::Light, c_ChunkSize> m_Lighting;
  GlobalIndex m_GlobalIndex;
  std::atomic<uint16_t> m_NonOpaqueFaces;

  void setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType);
  void setBlockType(const BlockIndex& blockIndex, Block::Type blockType);

  void setComposition(Array3D<Block::Type, c_ChunkSize>&& composition);
  void setLighting(Array3D<Block::Light, c_ChunkSize>&& lighting);
  void determineOpacity();

  void update(bool hasMesh);
  void reset();

  _Acquires_lock_(return) std::lock_guard<std::mutex> acquireLock() const;

private:
  class Voxel
  {
  public:
    Voxel();
    Voxel(uint32_t voxelData, uint32_t quadData1, uint32_t quadData2, uint32_t vertexSunlight);

    blockIndex_t i() const;
    blockIndex_t j() const;
    blockIndex_t k() const;
    BlockIndex index() const;

  private:
    uint32_t m_VoxelData;
    uint32_t m_QuadData1;
    uint32_t m_QuadData2;
    uint32_t m_Sunlight;
  };

  class DrawCommand : public Engine::MultiDrawCommand<GlobalIndex, DrawCommand>
  {
  public:
    DrawCommand(const GlobalIndex& chunkIndex);
    DrawCommand(const GlobalIndex& chunkIndex, std::vector<Voxel>&& mesh);

    DrawCommand(DrawCommand&& other) noexcept;
    DrawCommand& operator=(DrawCommand&& other) noexcept;

    // Copy operators deleted to prevent copying of mesh data
    DrawCommand(const DrawCommand& other) = delete;
    DrawCommand& operator=(const DrawCommand& other) = delete;

    bool operator==(const DrawCommand& other) const;

    const void* vertexData() const;

    /*
      Ensures voxels are sorted based on their distance to the given position, back to front.
      This operation is surprisely cheap, and can usually be done many times per frame.
    */
    void sortVoxels(const GlobalIndex& originIndex, const Vec3& position);

  private:
    std::vector<Voxel> m_Mesh;
    BlockIndex m_SortState;
  };

  friend class ChunkFiller;
  friend class ChunkManager;
  friend class ChunkContainer;
  friend struct std::hash<DrawCommand>;
};

static constexpr int s = sizeof(Chunk);

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