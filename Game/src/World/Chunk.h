#pragma once
#include "Block/Block.h"
#include "Block/BlockIDs.h"

struct HeightMap
{
  int64_t chunkX;
  int64_t chunkZ;
  std::array<std::array<float, 32>, 32> terrainHeights;
};

class Chunk
{
public:
  Chunk();
  Chunk(const std::array<int64_t, 3>& chunkIndex);
  ~Chunk();

  Chunk(const Chunk& other) = delete;
  Chunk& operator=(const Chunk& other) = delete;

  Chunk(Chunk&& other) = default;
  Chunk& operator=(Chunk&& other) = default;

  void load(HeightMap heightMap);
  void generateMesh();
  void bindBuffers() const;

  const std::array<int64_t, 3>& getIndex() const { return m_ChunkIndex; }
  glm::vec3 position() const { return glm::vec3(m_ChunkIndex[0] * Length(), m_ChunkIndex[1] * Length(), m_ChunkIndex[2] * Length()); }
  glm::vec3 center() const { return glm::vec3(m_ChunkIndex[0] * 1.5f * Length(), m_ChunkIndex[1] * 1.5f * Length(), m_ChunkIndex[2] * 1.5f * Length()); }

  void setBlockType(uint8_t i, uint8_t j, uint8_t k, BlockType blockType);
  BlockType getBlockType(uint8_t i, uint8_t j, uint8_t k) const;
  glm::vec3 blockPosition(uint8_t i, uint8_t j, uint8_t k) const;

  const Chunk* getNeighbor(BlockFace face) const { return m_Neighbors[static_cast<uint8_t>(face)]; }
  void setNeighbor(BlockFace face, Chunk* chunk) { m_Neighbors[static_cast<uint8_t>(face)] = chunk; }

  const std::vector<uint32_t>& getMesh() const { return m_Mesh; }
  const Engine::Shared<Engine::VertexArray>& getVertexArray() const { return m_MeshVertexArray; }

  bool isEmpty() const { return m_Empty; }
  bool isFaceOpaque(BlockFace face) const { return m_FaceIsOpaque[static_cast<uint8_t>(face)]; }
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
    Optimized
  };

  // Size and dimension
  static constexpr uint8_t s_ChunkSize = 32;
  static constexpr float s_ChunkLength = s_ChunkSize * Block::Length();
  static constexpr uint32_t s_ChunkTotalBlocks = s_ChunkSize * s_ChunkSize * s_ChunkSize;

  // Position and composition
  std::array<int64_t, 3> m_ChunkIndex;
  std::unique_ptr<BlockType[]> m_ChunkComposition = nullptr;
  bool m_Empty = true;
  bool m_Solid = true;
  std::array<bool, 6> m_FaceIsOpaque = { true, true, true, true, true, true };

  // Mesh data
  MeshState m_MeshState = MeshState::NotGenerated;
  std::vector<uint32_t> m_Mesh;
  Engine::Shared<Engine::VertexArray> m_MeshVertexArray;
  Engine::Shared<Engine::VertexBuffer> m_MeshVertexBuffer;
  static Engine::Shared<Engine::IndexBuffer> s_MeshIndexBuffer;

  // Chunk neighbor data
  std::array<Chunk*, 6> m_Neighbors{};

  void generateVertexArray();

  bool isBlockNeighborInAnotherChunk(uint8_t i, uint8_t j, uint8_t k, BlockFace face);
  bool isBlockNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, BlockFace face);
};