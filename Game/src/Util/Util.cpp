#include "GMpch.h"
#include "Util.h"
#include <glm/gtc/matrix_access.hpp>

bool Util::IsInRange(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex, globalIndex_t range)
{
  GlobalIndex diff = chunkIndex - originIndex;
  return abs(diff.i) <= range && abs(diff.j) <= range && abs(diff.k) <= range;
}

bool Util::BlockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, Direction direction)
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
  return blockIndex[AxisOf(direction)] == chunkLimits[IsUpstream(direction)];
}

std::array<Vec4, 6> Util::CalculateViewFrustumPlanes(const Mat4& viewProjection)
{
  Vec4 row1 = glm::row(viewProjection, 0);
  Vec4 row2 = glm::row(viewProjection, 1);
  Vec4 row3 = glm::row(viewProjection, 2);
  Vec4 row4 = glm::row(viewProjection, 3);

  std::array<Vec4, 6> frustumPlanes{};
  frustumPlanes[0] = row4 + row1;   // Left plane
  frustumPlanes[1] = row4 - row1;   // Right plane
  frustumPlanes[2] = row4 + row2;   // Bottom plane
  frustumPlanes[3] = row4 - row2;   // Top plane
  frustumPlanes[4] = row4 + row3;   // Near plane
  frustumPlanes[5] = row4 - row3;   // Far plane

  return frustumPlanes;
}

bool Util::IsInFrustum(const Vec3& point, const std::array<Vec4, 6>& frustumPlanes)
{
  for (int planeID = 0; planeID < 5; ++planeID) // Skip far plane
    if (glm::dot(Vec4(point, 1.0), frustumPlanes[planeID]) < 0)
      return false;

  return true;
}

std::unordered_map<GlobalIndex, BlockBox> Util::PartitionBlockBox(const BlockBox& box, const GlobalIndex& origin)
{
  std::unordered_map<GlobalIndex, BlockBox> boxDecomposition;

  LocalBox localBox = BlockBoxToLocalBox(box);
  localBox.forEach([&box, &origin, &boxDecomposition](const LocalIndex& localIndex)
    {
      GlobalIndex chunkIndex = origin + static_cast<GlobalIndex>(localIndex);
      BlockBox localIntersection = BlockBox::Intersection(Chunk::Bounds(), box - Chunk::Size() * static_cast<BlockIndex>(localIndex));
      boxDecomposition[chunkIndex] = localIntersection;
    });

  return boxDecomposition;
}
