#include "Chunk.h"

Chunk::Chunk(const glm::vec3& chunkPosition, Block blockType)
  : m_ChunkPosition(chunkPosition)
{
  for (int i = 0; i < s_ChunkVolume; ++i)
    chunkComposition[i] = blockType;

  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
        if (getBlockAt(i, j, k) != Block::Air)
          for (uint8_t face = (uint8_t)BlockFace::Top; face <= (uint8_t)BlockFace::West; ++face)
          {
            // Check all faces for air blocks
            if (isNeighborTransparent(i, j, k, (BlockFace)face))
            {
              glm::vec3 relativePosition = { i * s_BlockSize, j * s_BlockSize, k * s_BlockSize };
              m_Mesh.push_back({ (BlockFace)face, relativePosition });
            }
          }
  m_MeshState = MeshState::Simple;
}

Block Chunk::getBlockAt(int64_t i, int64_t j, int64_t k)
{
  constexpr int64_t chunkSize = (int64_t)s_ChunkSize;
  return chunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

void Chunk::render()
{
  for (uint32_t i = 0; i < m_Mesh.size(); ++i)
    ChunkRenderer::DrawBlockFace(m_Mesh[i], m_ChunkPosition);
}

bool Chunk::isNeighborTransparent(int64_t i, int64_t j, int64_t k, BlockFace face)
{
  switch (face)
  {
  case BlockFace::Top:      return j == s_ChunkSize - 1 ? true : getBlockAt(i, j + 1, k) == Block::Air;
  case BlockFace::Bottom:   return j == 0               ? true : getBlockAt(i, j - 1, k) == Block::Air;
  case BlockFace::North:    return k == s_ChunkSize - 1 ? true : getBlockAt(i, j, k + 1) == Block::Air;
  case BlockFace::South:    return k == 0               ? true : getBlockAt(i, j, k - 1) == Block::Air;
  case BlockFace::East:     return i == 0               ? true : getBlockAt(i - 1, j, k) == Block::Air;
  case BlockFace::West:     return i == s_ChunkSize - 1 ? true : getBlockAt(i + 1, j, k) == Block::Air;
  }
}
