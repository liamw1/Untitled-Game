#pragma once
#include "World/Chunk.h"

namespace Util
{
  /*
    Generates a (nearly) unique key for hash maps.
  */
  int CreateKey(const GlobalIndex& chunkIndex);
  int CreateKey(const Chunk* chunk);
  int Create2DMapKey(globalIndex_t chunkI, globalIndex_t chunkJ);

  bool IsInRangeOfPlayer(const GlobalIndex& chunkIndex, globalIndex_t range);
  bool IsInRangeOfPlayer(const Chunk* chunk, globalIndex_t range);

  bool BlockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, Block::Face face);

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
}