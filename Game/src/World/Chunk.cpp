#include "Chunk.h"
#include <Engine.h>

Chunk::Chunk()
  : m_ChunkIndices({0, 0, 0}),
    m_ChunkPosition({0.0f, 0.0f, 0.0f}),
    m_ChunkComposition(nullptr)
{
}

Chunk::Chunk(const std::array<int64_t, 3>& chunkIndices)
  : m_ChunkIndices(chunkIndices),
    m_ChunkPosition(m_ChunkIndices[0] * Length(), m_ChunkIndices[1] * Length(), m_ChunkIndices[2] * Length())
{
  m_ChunkComposition = std::make_unique<Block[]>(s_ChunkTotalBlocks);
}

void Chunk::load(Block blockType)
{
  EN_PROFILE_FUNCTION();

  for (int i = 0; i < s_ChunkTotalBlocks; ++i)
    m_ChunkComposition[i] = blockType;

  generateMesh();
}

void Chunk::generateMesh()
{
  EN_PROFILE_FUNCTION();

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

  // NOTE: Potential optimization by using reserve() for mesh vector
}

Block Chunk::getBlockAt(uint8_t i, uint8_t j, uint8_t k) const
{
  constexpr uint64_t chunkSize = (uint64_t)s_ChunkSize;
  return m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
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
