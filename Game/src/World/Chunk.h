#pragma once
#include "Indexing.h"
#include "Block/Block.h"
#include "Util/MultiDimArrays.h"
#include <Engine.h>

/*
  A class representing a NxNxN cube of blocks.

  IMPORTANT: Before calling uploadMesh or draw, the chunk's vertex array must be
             initialized. This only has to be done once, and should only be done
             for chunk objects that have a permanent location in memory, such as
             the chunk's stored in the chunk container.
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
  const GlobalIndex& getGlobalIndex() const { return m_GlobalIndex; }

  /*
    \returns The chunk's index relative to the origin chunk.
  */
  LocalIndex getLocalIndex() const;

  /*
    A chunk's anchor point is its bottom southeast vertex.
    Position given relative to the anchor of the origin chunk.
    Useful property:
    If the anchor point is denoted by A, then for any point
    X within the chunk, X_i >= A_i.
  */
  Vec3 anchorPosition() const;

  /*
    \returns The chunk's geometric center relative to origin chunk.
  */
  Vec3 center() const { return Chunk::Center(anchorPosition()); }

  bool empty() const { return !m_Composition; }

  Block::Type getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const;
  Block::Type getBlockType(const BlockIndex& blockIndex) const;

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

  static Vec3 Center(const Vec3& anchorPosition) { return anchorPosition + Chunk::Length() / 2; }
  static Vec3 AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex);

// All private functions require a lock on the chunk mutex.
private:
  static constexpr blockIndex_t c_ChunkSize = 32;

  mutable std::mutex m_Mutex;
  Array3D<Block::Type, c_ChunkSize> m_Composition;
  GlobalIndex m_GlobalIndex;
  std::atomic<uint16_t> m_NonOpaqueFaces;

  void setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType);
  void setBlockType(const BlockIndex& blockIndex, Block::Type blockType);

  void setData(Array3D<Block::Type, c_ChunkSize> composition);
  void determineOpacity();

  void update(bool hasMesh);
  void reset();

  _Acquires_lock_(return) std::lock_guard<std::mutex> acquireLock() const { return std::lock_guard(m_Mutex); };

  class Voxel
  {
  public:
    Voxel() = default;
    Voxel(uint32_t voxelData, uint32_t quadData1, uint32_t quadData2)
      : m_VoxelData(voxelData), m_QuadData1(quadData1), m_QuadData2(quadData2) {}


    blockIndex_t x() const;
    blockIndex_t y() const;
    blockIndex_t z() const;
    BlockIndex index() const;

  private:
    uint32_t m_VoxelData;
    uint32_t m_QuadData1;
    uint32_t m_QuadData2;
  };

  class DrawCommand : public Engine::MultiDrawCommand<GlobalIndex, DrawCommand>
  {
  public:
    DrawCommand(const GlobalIndex& chunkIndex)
      : Engine::MultiDrawCommand<GlobalIndex, DrawCommand>(chunkIndex, 0),
        m_Mesh() {}

    DrawCommand(const GlobalIndex& chunkIndex, std::vector<Voxel>&& mesh)
      : Engine::MultiDrawCommand<GlobalIndex, DrawCommand>(chunkIndex, static_cast<int>(mesh.size())),
        m_Mesh(std::move(mesh)) {}

    DrawCommand(const DrawCommand& other) = delete;
    DrawCommand& operator=(const DrawCommand& other) = delete;

    DrawCommand(DrawCommand&& other) noexcept = default;
    DrawCommand& operator=(DrawCommand&& other) noexcept = default;

    bool operator==(const DrawCommand& other) const { return m_ID == other.m_ID; }

    const void* vertexData() const { return m_Mesh.data(); }

    void sortVoxels(const GlobalIndex& originIndex, const Vec3& playerPosition);

  private:
    std::vector<Voxel> m_Mesh;
    BlockIndex m_SortState;
  };

  Engine::MultiDrawArray<DrawCommand>::Identifier test;

  friend class ChunkManager;
  friend class ChunkContainer;
  friend struct std::hash<DrawCommand>;
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