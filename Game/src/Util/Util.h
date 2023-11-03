#pragma once
#include "World/Chunk.h"

namespace util
{
  bool isInRange(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex, globalIndex_t range);

  bool blockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, eng::math::Direction direction);

  /*
  Uses algorithm described in
  https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

  \returns An array of vectors representing the view frustum planes.
           For a plane of the form Ax + By + Cz + D = 0,
           its corresponding vector is {A, B, C, D}.
  */
  std::array<eng::math::Vec4, 6> calculateViewFrustumPlanes(const eng::math::Mat4& viewProjection);

  /*
    \returns True if the given point is inside the given set of frustum planes.
             Could be any frustum, not necessarily the view frustum.
  */
  bool isInFrustum(const eng::math::Vec3& point, const std::array<eng::math::Vec4, 6>& frustumPlanes);

  std::unordered_map<GlobalIndex, BlockBox> partitionBlockBox(const BlockBox& box, const GlobalIndex& origin);

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
}