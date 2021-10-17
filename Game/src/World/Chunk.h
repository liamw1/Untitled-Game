#pragma once
#include "Block/Block.h"
#include "Block/BlockIDs.h"

using ChunkIndex = std::array<int64_t, 3>;

struct HeightMap
{
  int64_t chunkX;
  int64_t chunkY;
  std::array<std::array<float, 32>, 32> terrainHeights;
};

class Chunk
{
public:
  Chunk();
  Chunk(const ChunkIndex& chunkIndex);
  ~Chunk();

  Chunk(const Chunk& other) = delete;
  Chunk& operator=(const Chunk& other) = delete;

  Chunk(Chunk&& other) noexcept;
  Chunk& operator=(Chunk&& other) noexcept;

  void load(HeightMap heightMap);
  void generateMesh();
  void update();
  void bindBuffers() const;
  void reset();

  const ChunkIndex& getIndex() const { return m_ChunkIndex; }
  glm::vec3 position() const { return glm::vec3(m_ChunkIndex[0] * Length(), m_ChunkIndex[1] * Length(), m_ChunkIndex[2] * Length()); }
  glm::vec3 center() const { return glm::vec3(m_ChunkIndex[0] * 1.5f * Length(), m_ChunkIndex[1] * 1.5f * Length(), m_ChunkIndex[2] * 1.5f * Length()); }

  void setBlockType(uint8_t i, uint8_t j, uint8_t k, BlockType blockType);
  BlockType getBlockType(uint8_t i, uint8_t j, uint8_t k) const;
  glm::vec3 blockPosition(uint8_t i, uint8_t j, uint8_t k) const;

  Chunk* const getNeighbor(BlockFace face) const { return m_Neighbors[static_cast<uint8_t>(face)]; }
  void setNeighbor(BlockFace face, Chunk* chunk);

  const std::vector<uint32_t>& getMesh() const { return m_Mesh; }
  const Engine::Shared<Engine::VertexArray>& getVertexArray() const { return m_MeshVertexArray; }

  bool isEmpty() const { return m_Empty; }
  bool isFaceOpaque(BlockFace face) const { return m_FaceIsOpaque[static_cast<uint8_t>(face)]; }

  static constexpr uint8_t Size() { return s_ChunkSize; }
  static constexpr float Length() { return s_ChunkLength; }
  static constexpr uint32_t TotalBlocks() { return s_ChunkTotalBlocks; }
  static const ChunkIndex GetPlayerChunkIndex(const glm::vec3& playerPosition);

  static void InitializeIndexBuffer();

private:
  enum class MeshState : uint8_t
  {
    NotGenerated = 0,
    NeedsUpdate,
    Simple,
    Optimized
  };

  // Size and dimension
  static constexpr uint8_t s_ChunkSize = 32;
  static constexpr float s_ChunkLength = s_ChunkSize * Block::Length();
  static constexpr uint32_t s_ChunkTotalBlocks = s_ChunkSize * s_ChunkSize * s_ChunkSize;

  // Position and composition
  ChunkIndex m_ChunkIndex;
  Engine::Unique<BlockType[]> m_ChunkComposition = nullptr;
  bool m_Empty = true;
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

  void sendAddressUpdate();
  void excise();
};