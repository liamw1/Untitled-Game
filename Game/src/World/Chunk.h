#pragma once
#include <Engine.h>
#include "Block/Block.h"
#include "Block/BlockIDs.h"

class Chunk
{
public:
  Chunk();
  Chunk(const std::array<int64_t, 3>& chunkIndex);

  void load(Block blockType);
  void unload();

  void generateMesh();
  void bindBuffers() const;

  const std::array<int64_t, 3>& getIndex() const { return m_ChunkIndex; }
  glm::vec3 position() const { return glm::vec3(m_ChunkIndex[0] * Length(), m_ChunkIndex[1] * Length(), m_ChunkIndex[2] * Length()); }
  glm::vec3 center() const { return glm::vec3(m_ChunkIndex[0] * 1.5f * Length(), m_ChunkIndex[1] * 1.5f * Length(), m_ChunkIndex[2] * 1.5f * Length()); }

  Block getBlockAt(uint8_t i, uint8_t j, uint8_t k) const;

  const Chunk* getNeighbor(uint8_t face) const { return m_Neighbors[face]; }
  void setNeighbor(uint8_t face, Chunk* chunk) { m_Neighbors[face] = chunk; }

  const std::vector<uint32_t>& getMesh() const { return m_Mesh; }
  const Engine::Shared<Engine::VertexArray>& getVertexArray() const { return m_MeshVertexArray; }

  bool isEmpty() const { return m_Empty; }
  bool allNeighborsLoaded() const;
  bool isMeshGenerated() const { return m_MeshState != MeshState::NotGenerated; }

  static constexpr uint8_t Size() { return s_ChunkSize; }
  static constexpr float Length() { return s_ChunkLength; }
  static constexpr uint32_t TotalBlocks() { return s_ChunkTotalBlocks; }
  static const std::array<int64_t, 3> GetPlayerChunkIndex(const glm::vec3& playerPosition);

  static void InitializeIndexBuffer();

private:
  enum class MeshState : uint8_t
  {
    NotGenerated = 0,
    Simple,
    AllNeighborsAccountedFor,
    Optimized
  };

  // Size and dimension
  static constexpr uint8_t s_ChunkSize = 32;
  static constexpr float s_ChunkLength = s_ChunkSize * s_BlockSize;
  static constexpr uint32_t s_ChunkTotalBlocks = s_ChunkSize * s_ChunkSize * s_ChunkSize;

  // Position and composition
  std::array<int64_t, 3> m_ChunkIndex;
  std::unique_ptr<Block[]> m_ChunkComposition;
  bool m_Empty = true;

  // Mesh data
  MeshState m_MeshState = MeshState::NotGenerated;
  std::vector<uint32_t> m_Mesh;
  Engine::Shared<Engine::VertexArray> m_MeshVertexArray;
  Engine::Shared<Engine::VertexBuffer> m_MeshVertexBuffer;
  static Engine::Shared<Engine::IndexBuffer> s_MeshIndexBuffer;

  // Chunk neighbor data
  std::array<Chunk*, 6> m_Neighbors{};

  void generateVertexArray();

  bool isOnBoundary(uint8_t i, uint8_t j, uint8_t k, uint8_t face);
  bool isNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, uint8_t face);
};