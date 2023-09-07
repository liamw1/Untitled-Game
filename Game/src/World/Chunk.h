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
  template<typename T>
  using ArrayRect = ArrayRect<T, blockIndex_t, 0, c_ChunkSize>;

  template<typename T>
  using ArrayBox = ArrayBox<T, blockIndex_t, 0, c_ChunkSize>;

  template<typename T>
  using ProtectedArrayBox = Engine::Threads::ProtectedArrayBox<T, blockIndex_t, 0, c_ChunkSize>;

public:
  Chunk() = delete;
  Chunk(const GlobalIndex& chunkIndex);

  ProtectedArrayBox<Block::Type>& composition();
  const ProtectedArrayBox<Block::Type>& composition() const;

  ProtectedArrayBox<Block::Light>& lighting();
  const ProtectedArrayBox<Block::Light>& lighting() const;

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

  void setComposition(ArrayBox<Block::Type>&& composition);
  void setLighting(ArrayBox<Block::Light>&& lighting);
  void determineOpacity();

  void update();

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

private:
  ProtectedArrayBox<Block::Type> m_Composition;
  ProtectedArrayBox<Block::Light> m_Lighting;
  std::atomic<uint16_t> m_NonOpaqueFaces;
};

struct ChunkWithLock
{
  std::shared_ptr<Chunk> chunk;
  std::unique_lock<std::mutex> lock;
};