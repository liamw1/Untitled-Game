#include "GMpch.h"
#include "Util.h"
#include "Player/Player.h"

int Util::CreateKey(const GlobalIndex& chunkIndex)
{
  return chunkIndex.i % bitUi32(10) + bitUi32(10) * (chunkIndex.j % bitUi32(10)) + bitUi32(20) * (chunkIndex.k % bitUi32(10));
}

int Util::CreateKey(const Chunk* chunk)
{
  return CreateKey(chunk->getGlobalIndex());
}

int Util::Create2DMapKey(globalIndex_t chunkI, globalIndex_t chunkJ)
{
  return CreateKey(GlobalIndex(chunkI, chunkJ, 0));
}

bool Util::IsInRangeOfPlayer(const GlobalIndex& chunkIndex, globalIndex_t range)
{
  GlobalIndex originIndex = Player::OriginIndex();

  for (int i = 0; i < 3; ++i)
    if (abs(chunkIndex[i] - originIndex[i]) > range)
      return false;
  return true;
}

bool Util::IsInRangeOfPlayer(const Chunk* chunk, globalIndex_t range)
{
  return IsInRangeOfPlayer(chunk->getGlobalIndex(), range);
}

bool Util::IsInRangeOfPlayer(const Chunk& chunk, globalIndex_t range)
{
  return IsInRangeOfPlayer(chunk.getGlobalIndex(), range);
}

bool Util::BlockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, Block::Face face)
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
  int faceID = static_cast<int>(face);
  int coordID = faceID / 2;

  return blockIndex[coordID] == chunkLimits[faceID % 2];
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
