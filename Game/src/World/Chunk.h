#pragma once
#include "Block/Block.h"

struct HeightMap
{
  int64_t chunkI;
  int64_t chunkJ;
  std::array<std::array<float, 32>, 32> terrainHeights;
};

struct ChunkIndex
{
  int64_t i;
  int64_t j;
  int64_t k;

  int64_t& operator[](int index)
  {
    EN_ASSERT(index >= 0 && index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  const int64_t& operator[](int index) const
  {
    EN_ASSERT(index >= 0 && index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  bool operator==(const ChunkIndex& other) const
  {
    return i == other.i && j == other.j && k == other.k;
  }
};

struct LocalIndex
{
  uint8_t i;
  uint8_t j;
  uint8_t k;

  uint8_t& operator[](int index)
  {
    EN_ASSERT(index >= 0 && index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  const uint8_t& operator[](int index) const
  {
    EN_ASSERT(index >= 0 && index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  bool operator==(const ChunkIndex& other) const
  {
    return i == other.i && j == other.j && k == other.k;
  }
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

  BlockType getBlockType(uint8_t i, uint8_t j, uint8_t k) const;
  BlockType getBlockType(const LocalIndex& blockIndex) const;
  BlockType getBlockType(const glm::vec3& position) const;
  void setBlockType(uint8_t i, uint8_t j, uint8_t k, BlockType blockType);
  void setBlockType(const LocalIndex& blockIndex, BlockType blockType);

  /*
    \returns The local block index of the block that the given position
             sits inside.  Assumes that given position is inside chunk.
  */
  LocalIndex blockIndexFromPos(const glm::vec3& position) const;

  /*
    Creates and fills composition array with blocks based on the given height map.
    Deletes composition array if all blocks are air.
  */
  void load(HeightMap heightMap);

  /*
    Generates simplistic mesh based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.
  */
  void generateMesh();

  /*
    Applies necessary updates to chunk.  
    Chunk may be re-meshed, so call conservatively.
  */
  void update();

  /*
    Binds buffers necessary for rendering.
  */
  void bindBuffers() const;

  /*
    Restors chunk to default state.
  */
  void reset();

  /*
    \returns The chunk's index, which identifies it uniquely.
  */
  const ChunkIndex& getIndex() const { return m_ChunkIndex; }

  /*
    A chunk's anchor point is its bottom southeast vertex.

    Useful property:
    If the anchor point is denoted by A, then for any point
    X within the chunk, A_i <= X_i.
  */
  glm::vec3 anchorPoint() const { return glm::vec3(m_ChunkIndex.i * Length(), m_ChunkIndex.j * Length(), m_ChunkIndex.k * Length()); }

  /*
    \returns The chunk's geometric center.
  */
  glm::vec3 center() const { return anchorPoint() + Length() / 2; }

  /*
    \returns A pointer to the neighboring chunk in the specified direction.
             Will return nullptr if neighbor has not been loaded yet. 
    
    Neighbor is allowed to be modified, use this power responsibly!
  */
  Chunk* const getNeighbor(BlockFace face) const { return m_Neighbors[static_cast<uint8_t>(face)]; }

  /*
    Sets neighboring chunk in the specified direction.
    Upon use, a flag is set to re-mesh chunk when appropriate.
  */
  void setNeighbor(BlockFace face, Chunk* chunk);

  /*
    \returns A vector of compressed vertex data that represents the chunk's mesh.

    Compresed format is follows,
     bits 0-17:  Relative position of vertex within chunk (3-comps, 6 bits each)
     bits 18-20: Normal direction of quad (follows BlockFace convention)
     bits 21-22: Texture coordinate index (see BlockFace.glsl for details)
     bits 23-31: Texure ID
  */
  const std::vector<uint32_t>& getMesh() const { return m_Mesh; }
  const Engine::Shared<Engine::VertexArray>& getVertexArray() const { return m_MeshVertexArray; }

  bool isEmpty() const { return m_ChunkComposition == nullptr; }
  bool isFaceOpaque(BlockFace face) const { return m_FaceIsOpaque[static_cast<uint8_t>(face)]; }

  static constexpr uint8_t Size() { return s_ChunkSize; }
  static constexpr float Length() { return s_ChunkLength; }
  static constexpr uint32_t TotalBlocks() { return s_ChunkTotalBlocks; }
  static const ChunkIndex ChunkIndexFromPos(const glm::vec3& position);

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

  /*
    For updating neighbors if chunk moves to a different
    location in memory.
  */
  void sendAddressUpdate();

  /*
    Severs communication with neighboring chunks.
  */
  void excise();
};