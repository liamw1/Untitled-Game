#pragma once
#include "World/Chunk.h"

namespace Util
{
  bool IsInRange(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex, globalIndex_t range);

  bool BlockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, Direction direction);

  /*
  Uses algorithm described in
  https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

  \returns An array of vectors representing the view frustum planes.
           For a plane of the form Ax + By + Cz + D = 0,
           its corresponding vector is {A, B, C, D}.
  */
  std::array<Vec4, 6> CalculateViewFrustumPlanes(const Mat4& viewProjection);

  /*
    \returns True if the given point is inside the given set of frustum planes.
             Could be any frustum, not necessarily the view frustum.
  */
  bool IsInFrustum(const Vec3& point, const std::array<Vec4, 6>& frustumPlanes);

  std::unordered_map<GlobalIndex, BlockBox> PartitionBlockBox(const BlockBox& box, const GlobalIndex& origin);

  constexpr LocalBox BlockBoxToLocalBox(const BlockBox& box)
  {
    LocalBox chunkBox = static_cast<LocalBox>(box) / Chunk::Size();
    for (Axis axis : Axes())
    {
      if (box.min[axis] < 0)
        chunkBox.min[axis]--;
      if (box.max[axis] < 0)
        chunkBox.max[axis]--;
    }
    return chunkBox;
  }
}