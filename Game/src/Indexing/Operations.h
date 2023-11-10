#pragma once
#include "World\Chunk.h"

constexpr bool isInRange(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex, globalIndex_t range)
{
  GlobalIndex diff = chunkIndex - originIndex;
  return eng::math::abs(diff.i) <= range && eng::math::abs(diff.j) <= range && eng::math::abs(diff.k) <= range;
}

constexpr bool blockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, eng::math::Direction direction)
{
  blockIndex_t chunkLimit = isUpstream(direction) ? Chunk::Size() - 1 : 0;
  return blockIndex[axisOf(direction)] == chunkLimit;
}

constexpr LocalBox blockBoxToLocalBox(const BlockBox& box)
{
  LocalBox chunkBox = box.upcast<localIndex_t>() / Chunk::Size();
  for (eng::math::Axis axis : eng::math::Axes())
  {
    if (box.min[axis] < 0)
      chunkBox.min[axis]--;
    if (box.max[axis] < 0)
      chunkBox.max[axis]--;
  }
  return chunkBox;
}

constexpr std::vector<std::pair<LocalIndex, BlockBox>> partitionBlockBox(const BlockBox& box)
{
  std::vector<std::pair<LocalIndex, BlockBox>> boxPartitioning;
  blockBoxToLocalBox(box).forEach([&box, &boxPartitioning](const LocalIndex& localIndex)
  {
    BlockBox localIntersection = BlockBox::Intersection(Chunk::Bounds(), box - Chunk::Size() * localIndex.checkedCast<blockIndex_t>());
    boxPartitioning.emplace_back(localIndex, localIntersection);
  });
  return boxPartitioning;
}

constexpr std::array<BlockBox, 26> decomposeBlockBoxBoundary(const BlockBox& box)
{
  i32 arrayIndex = 0;
  std::array<BlockBox, 26> boxDecomposition;
  for (const BlockBox& faceInterior : eng::math::FaceInteriors(box))
    boxDecomposition[arrayIndex++] = faceInterior;
  for (const BlockBox& edgeInterior : eng::math::EdgeInteriors(box))
    boxDecomposition[arrayIndex++] = edgeInterior;
  for (const BlockBox& corner : eng::math::Corners(box))
    boxDecomposition[arrayIndex++] = corner;
  return boxDecomposition;
}

constexpr LocalBox affectedChunks(const BlockBox& chunkSection, blockIndex_t influenceRadius = 1)
{
  BlockBox affectedBlocks = chunkSection.expand(influenceRadius);
  return blockBoxToLocalBox(affectedBlocks);
}