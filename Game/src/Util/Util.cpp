#include "GMpch.h"
#include "Util.h"
#include <glm/gtc/matrix_access.hpp>

namespace util
{
  bool isInRange(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex, globalIndex_t range)
  {
    GlobalIndex diff = chunkIndex - originIndex;
    return std::abs(diff.i) <= range && std::abs(diff.j) <= range && std::abs(diff.k) <= range;
  }
  
  bool blockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, eng::math::Direction direction)
  {
    static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
    return blockIndex[AxisOf(direction)] == chunkLimits[IsUpstream(direction)];
  }
  
  std::array<eng::math::Vec4, 6> calculateViewFrustumPlanes(const eng::math::Mat4& viewProjection)
  {
    eng::math::Vec4 row1 = glm::row(viewProjection, 0);
    eng::math::Vec4 row2 = glm::row(viewProjection, 1);
    eng::math::Vec4 row3 = glm::row(viewProjection, 2);
    eng::math::Vec4 row4 = glm::row(viewProjection, 3);
  
    std::array<eng::math::Vec4, 6> frustumPlanes{};
    frustumPlanes[0] = row4 + row1;   // Left plane
    frustumPlanes[1] = row4 - row1;   // Right plane
    frustumPlanes[2] = row4 + row2;   // Bottom plane
    frustumPlanes[3] = row4 - row2;   // Top plane
    frustumPlanes[4] = row4 + row3;   // Near plane
    frustumPlanes[5] = row4 - row3;   // Far plane
  
    return frustumPlanes;
  }
  
  bool isInFrustum(const eng::math::Vec3& point, const std::array<eng::math::Vec4, 6>& frustumPlanes)
  {
    for (int planeID = 0; planeID < 5; ++planeID) // Skip far plane
      if (glm::dot(eng::math::Vec4(point, 1.0), frustumPlanes[planeID]) < 0)
        return false;
  
    return true;
  }
  
  std::unordered_map<GlobalIndex, BlockBox> partitionBlockBox(const BlockBox& box, const GlobalIndex& origin)
  {
    std::unordered_map<GlobalIndex, BlockBox> boxDecomposition;
  
    LocalBox localBox = blockBoxToLocalBox(box);
    localBox.forEach([&box, &origin, &boxDecomposition](const LocalIndex& localIndex)
      {
        GlobalIndex chunkIndex = origin + static_cast<GlobalIndex>(localIndex);
        BlockBox localIntersection = BlockBox::Intersection(Chunk::Bounds(), box - Chunk::Size() * static_cast<BlockIndex>(localIndex));
        boxDecomposition[chunkIndex] = localIntersection;
      });
  
    return boxDecomposition;
  }
}