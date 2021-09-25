#include "Chunk.h"
#include "ChunkRenderer.h"
#include "Block/Block.h"

Chunk::Chunk(const glm::vec3& chunkPosition, Block blockType)
  : m_ChunkPosition(chunkPosition)
{
  for (int i = 0; i < s_ChunkVolume; ++i)
    chunkComposition[i] = blockType;
}

Block Chunk::getBlockAt(int64_t i, int64_t j, int64_t k)
{
  constexpr int64_t chunkSize = (int64_t)s_ChunkSize;
  return chunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

void Chunk::render()
{
  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
      {
        glm::vec3 centerPosition = { i * s_BlockSize, j * s_BlockSize, k * s_BlockSize };
        for (uint8_t face = BlockFace::Top; face < BlockFace::End; ++face)
          ChunkRenderer::DrawBlockFace({ face, centerPosition }, m_ChunkPosition);
      }
}
