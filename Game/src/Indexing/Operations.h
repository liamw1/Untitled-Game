#pragma once
#include "GlobalParameters.h"
#include "World/Chunk/Chunk.h"

constexpr bool isInRange(const GlobalIndex& index, const GlobalIndex& originIndex, globalIndex_t range)
{
  GlobalIndex diff = index - originIndex;
  return eng::math::abs(diff.i) <= range && eng::math::abs(diff.j) <= range && eng::math::abs(diff.k) <= range;
}

constexpr bool blockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, eng::math::Direction direction)
{
  blockIndex_t chunkLimit = isUpstream(direction) ? Chunk::Size() - 1 : 0;
  return blockIndex[axisOf(direction)] == chunkLimit;
}

constexpr eng::math::Vec3 indexPosition(const GlobalIndex& index, const GlobalIndex& originIndex)
{
  return Chunk::Length() * static_cast<eng::math::Vec3>(index - originIndex);
}

constexpr eng::math::Vec3 indexCenter(const GlobalIndex& index, const GlobalIndex& originIndex)
{
  return indexPosition(index, originIndex) + Chunk::Length() / 2;
}

constexpr LocalBox blockBoxToLocalBox(const BlockBox& box)
{
  return box.upcast<localIndex_t>().flooredDivide(Chunk::Size());
}

constexpr std::vector<std::pair<LocalIndex, BlockBox>> partitionBlockBox(const BlockBox& box)
{
  std::vector<std::pair<LocalIndex, BlockBox>> boxPartitioning;
  for (const LocalIndex& localIndex : blockBoxToLocalBox(box))
  {
    BlockBox localIntersection = BlockBox::Intersection(Chunk::Bounds(), box - Chunk::Size() * localIndex.checkedCast<blockIndex_t>());
    boxPartitioning.emplace_back(localIndex, localIntersection);
  }
  return boxPartitioning;
}

constexpr std::array<BlockBox, 26> decomposeBlockBoxBoundary(const BlockBox& box)
{
  std::array<BlockBox, 26> boxDecomposition;
  eng::algo::copy(eng::math::FaceInteriors(box), boxDecomposition.begin());
  eng::algo::copy(eng::math::EdgeInteriors(box), boxDecomposition.begin() + 6);
  eng::algo::copy(eng::math::Corners(box), boxDecomposition.begin() + 6 + 12);
  return boxDecomposition;
}

constexpr LocalBox affectedChunks(const BlockBox& chunkSection, blockIndex_t influenceRadius = 1)
{
  BlockBox affectedBlocks = chunkSection.expand(influenceRadius);
  return blockBoxToLocalBox(affectedBlocks);
}

constexpr LocalBox affectedChunks(const BlockIndex& blockIndex, blockIndex_t influenceRadius = 1)
{
  return affectedChunks(BlockBox(blockIndex, blockIndex), influenceRadius);
}