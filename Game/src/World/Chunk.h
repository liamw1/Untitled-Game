#pragma once
#include "Block/Block.h"
#include "Block/BlockIDs.h"
#include "ChunkRenderer.h"

class Chunk
{
public:
  Chunk() = delete;
  Chunk(const std::array<int64_t, 3>& chunkIndices);

  void load(Block blockType);
  void generateMesh();

  Block getBlockAt(uint8_t i, uint8_t j, uint8_t k) const;
  const std::array<int64_t, 3>& getIndices() const { return m_ChunkIndices; }
  const glm::vec3& getPosition() const { return m_ChunkPosition; }
  glm::vec3 center() const { return m_ChunkPosition + Length() / 2; }
  const std::vector<ChunkRenderer::BlockFaceParams>& getMesh() const { return m_Mesh; }

  bool isLoaded() const { return m_MeshState != MeshState::NotGenerated; }

  static constexpr uint8_t Size() { return s_ChunkSize; }
  static constexpr float Length() { return s_ChunkLength; }
  static constexpr uint32_t TotalBlocks() { return s_ChunkTotalBlocks; }
  static constexpr std::array<int64_t, 3> GetPlayerChunk(const glm::vec3& playerPosition) { return { (int64_t)(playerPosition.x / Chunk::Length()), (int64_t)(playerPosition.y / Chunk::Length()) , (int64_t)(playerPosition.z / Chunk::Length()) }; }

private:
  enum class MeshState : uint8_t
  {
    NotGenerated = 0,
    Simple,
    AllNeighborsAccountedFor,
    Optimized
  };

  static constexpr uint8_t s_ChunkSize = 32;
  static constexpr float s_ChunkLength = s_ChunkSize * s_BlockSize;
  static constexpr uint32_t s_ChunkTotalBlocks = s_ChunkSize * s_ChunkSize * s_ChunkSize;

  std::array<int64_t, 3> m_ChunkIndices;
  glm::vec3 m_ChunkPosition;
  std::unique_ptr<Block[]> m_ChunkComposition;

  MeshState m_MeshState = MeshState::NotGenerated;
  std::vector<ChunkRenderer::BlockFaceParams> m_Mesh;

  bool isOnBoundary(uint8_t i, uint8_t j, uint8_t k, uint8_t face);
  bool isNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, uint8_t face);
};