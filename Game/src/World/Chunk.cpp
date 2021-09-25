#include "Chunk.h"

Chunk::Chunk()
  : Chunk(glm::vec3(0.0f), Block::Sand) {}

Chunk::Chunk(const glm::vec3& chunkPosition, Block blockType)
  : m_ChunkPosition(chunkPosition)
{
  {
    EN_PROFILE_SCOPE("Chunk loading");

    for (int i = 0; i < s_ChunkVolume; ++i)
      chunkComposition[i] = blockType;
  }

  {
    EN_PROFILE_SCOPE("Mesh generation");

    for (uint8_t i = 0; i < s_ChunkSize; ++i)
      for (uint8_t j = 0; j < s_ChunkSize; ++j)
        for (uint8_t k = 0; k < s_ChunkSize; ++k)
          if (getBlockAt(i, j, k) != Block::Air)
            for (uint8_t face = (uint8_t)BlockFace::Top; face <= (uint8_t)BlockFace::West; ++face)
            {
              // Check all faces for air blocks
              if (isNeighborTransparent(i, j, k, face))
              {
                glm::vec3 relativePosition = { i * s_BlockSize, j * s_BlockSize, k * s_BlockSize };
                m_Mesh.push_back({ (BlockFace)face, relativePosition });
              }
            }
    m_MeshState = MeshState::Simple;
  }

  // NOTE: Potential optimization by using reserve() for mesh vector
}

Block Chunk::getBlockAt(uint8_t i, uint8_t j, uint8_t k)
{
  constexpr uint64_t chunkSize = (uint64_t)s_ChunkSize;
  return chunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

void Chunk::render()
{
  EN_PROFILE_FUNCTION();

  for (uint32_t i = 0; i < m_Mesh.size(); ++i)
    ChunkRenderer::DrawBlockFace(m_Mesh[i], m_ChunkPosition);
}

bool Chunk::isOnBoundary(uint8_t i, uint8_t j, uint8_t k, uint8_t face)
{
  using func = bool(*)(uint8_t, uint8_t, uint8_t, uint8_t);
  static const func isOnChunkSide[6] = { [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return j == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return j == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return k == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return k == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return i == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return i == s_ChunkSize - 1; } };

  return isOnChunkSide[face](i, j, k, face);
}

bool Chunk::isNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, uint8_t face)
{
  static constexpr int8_t normals[6][3] = { { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1}, { -1, 0, 0}, { 1, 0, 0} };
                                       //       Top        Bottom       North       South         East         West

  if (isOnBoundary(i, j, k, face))
    return true;
  else
    return getBlockAt(i + normals[face][0], j + normals[face][1], k + normals[face][2]) == Block::Air;
}
