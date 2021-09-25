#pragma once
#include <Engine.h>
#include "Block/Block.h"
#include "Block/BlockIDs.h"
#include "ChunkRenderer.h"

class Chunk
{
public:
  Chunk();
  Chunk(const glm::vec3& chunkPosition, Block blockType);

  Block getBlockAt(uint8_t i, uint8_t j, uint8_t k);

  void render();

  static constexpr uint8_t Size() { return s_ChunkSize; }
  static constexpr uint32_t Volume() { return s_ChunkVolume; }

private:
  enum class MeshState : uint8_t
  {
    NotGenerated = 0,
    Simple,
    AllNeighborsAccountedFor,
    Optimized
  };

  static constexpr uint8_t s_ChunkSize = 32;
  static constexpr uint32_t s_ChunkVolume = s_ChunkSize * s_ChunkSize * s_ChunkSize;

  const glm::vec3 m_ChunkPosition;
  std::array<Block, s_ChunkVolume> chunkComposition;

  MeshState m_MeshState = MeshState::NotGenerated;
  std::vector<ChunkRenderer::BlockFaceParams> m_Mesh;

  bool isOnBoundary(uint8_t i, uint8_t j, uint8_t k, uint8_t face);
  bool isNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, uint8_t face);
};