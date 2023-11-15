#pragma once
#include "Indexing/Definitions.h"
#include "Block/Block.h"

/*
  A class representing a NxNxN cube of blocks.
*/
class Chunk : private eng::NonCopyable, eng::NonMovable
{
  static constexpr blockIndex_t c_ChunkSize = 32;

  ProtectedBlockArrayBox<block::Type> m_Composition;
  ProtectedBlockArrayBox<block::Light> m_Lighting;
  std::atomic<u16> m_NonOpaqueFaces;
  GlobalIndex m_GlobalIndex;

public:
  Chunk() = delete;
  explicit Chunk(const GlobalIndex& chunkIndex);

  const GlobalIndex& globalIndex() const;

  ProtectedBlockArrayBox<block::Type>& composition();
  const ProtectedBlockArrayBox<block::Type>& composition() const;

  ProtectedBlockArrayBox<block::Light>& lighting();
  const ProtectedBlockArrayBox<block::Light>& lighting() const;

  template<typename T>
  const ProtectedBlockArrayBox<T>& data() const
  {
    if constexpr (std::is_same_v<T, block::Type>)
      return composition();
    else if constexpr (std::is_same_v<T, block::Light>)
      return lighting();
    else
      static_assert(eng::AlwaysFalse<T>, "Chunk does not store this type!");
  }

  /*
    \returns Whether or not a given chunk face has transparent blocks. Useful for deciding which chunks should be loaded
    into memory.

    WARNING: This function only reflects the opacity state of the chunk when determineOpacity() was last called. If any
    changes to chunk composition occured since then, this function may not be accurate. This is because determineOpacity
    is a costly operation (at least relative to the cost of changing a single block), so we prefer to only compute opacity
    when the chunk is updated via setComposition or update. Use with caution.
  */
  bool isFaceOpaque(eng::math::Direction face) const;

  void setComposition(BlockArrayBox<block::Type>&& composition);
  void setLighting(BlockArrayBox<block::Light>&& lighting);
  void determineOpacity();

  void update();

  /*
    \returns The chunk's geometric center relative to origin chunk.
  */
  static eng::math::Vec3 Center(const eng::math::Vec3& anchorPosition);

  /*
    A chunk's anchor point is its bottom southeast vertex.
    Position given relative to the anchor of the origin chunk.
    Useful property:
    If the anchor point is denoted by A, then for any point
    X within the chunk, X_i >= A_i.
  */
  static eng::math::Vec3 AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex);

  static constexpr blockIndex_t Size() { return c_ChunkSize; }
  static constexpr length_t Length() { return block::length() * Size(); }
  static constexpr i32 TotalBlocks() { return eng::math::cube(Size()); }
  static constexpr BlockBox Bounds() { return BlockBox(0, Size() - 1); }
  static constexpr BlockRect Bounds2D() { return BlockRect(0, Size() - 1); }
  static constexpr GlobalBox Stencil(const GlobalIndex& chunkIndex) { return GlobalBox(-1, 1) + chunkIndex; }
};