#pragma once
#include "Block/Block.h"

struct Vertex
{
  Float3 position;
};

/*
  Structs for indexing of blocks and chunks.
*/
template<typename intType>
struct Index3D
{
  intType i;
  intType j;
  intType k;

  intType& operator[](uint8_t index)
  {
    EN_ASSERT(index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  const intType& operator[](uint8_t index) const
  {
    EN_ASSERT(index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  constexpr bool operator==(const Index3D<intType>& other) const { return i == other.i && j == other.j && k == other.k; }
  constexpr Index3D<intType> operator+(const Index3D<intType>& other) const 
  { 
    return { static_cast<intType>(i + other.i), 
             static_cast<intType>(j + other.j), 
             static_cast<intType>(k + other.k) }; 
  }
  constexpr Index3D<intType> operator-(const Index3D<intType>& other) const 
  { 
    return { static_cast<intType>(i - other.i), 
             static_cast<intType>(j - other.j), 
             static_cast<intType>(k - other.k) }; 
  }
};

template<typename intType>
constexpr Index3D<intType> operator*(intType n, const Index3D<intType>& index) 
{ 
  return { static_cast<intType>(n * index.i), 
           static_cast<intType>(n * index.j), 
           static_cast<intType>(n * index.k) }; 
}

using blockIndex_t = uint8_t;
using localIndex_t = int16_t;
using globalIndex_t = int64_t;

using BlockIndex = Index3D<blockIndex_t>;
using LocalIndex = Index3D<localIndex_t>;
using GlobalIndex = Index3D<globalIndex_t>;

struct HeightMap
{
  globalIndex_t chunkI;
  globalIndex_t chunkJ;
  std::array<std::array<length_t, 32>, 32> terrainHeights;

  length_t maxHeight;
};

/*
  A large cube of blocks.

  Block indices refer to the positioning of blocks within a chunk.
  Local indices refer to the positioning of a chunk relative to the origin chunk.
  Global indices refer to the positioning of a chunk relative to the absolute origin of the world.
*/
class Chunk
{
public:
  Chunk();
  Chunk(const GlobalIndex& chunkIndex);
  ~Chunk();

  Chunk(const Chunk& other) = delete;
  Chunk& operator=(const Chunk& other) = delete;

  Chunk(Chunk&& other) noexcept;
  Chunk& operator=(Chunk&& other) noexcept;

  BlockType getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const;
  BlockType getBlockType(const BlockIndex& blockIndex) const;
  BlockType getBlockType(const Vec3& position) const;

  void removeBlock(const BlockIndex& blockIndex);
  void placeBlock(const BlockIndex& blockIndex, BlockFace face, BlockType blockType);

  /*
    \returns The local block index of the block that the given position
             sits inside.  Assumes that given position is inside chunk.
  */
  BlockIndex blockIndexFromPos(const Vec3& position) const;

  /*
    Creates and fills composition array with blocks based on the given height map.
    Deletes composition array if all blocks are air.
  */
  void load(HeightMap heightMap);

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
    \returns The chunk's index relative to the origin chunk.
  */
  LocalIndex getLocalIndex() const;

  /*
    \returns The chunk's global index, which identifies it uniquely.
  */
  const GlobalIndex& getGlobalIndex() const { return m_GlobalIndex; }

  /*
    A chunk's (local) anchor point is its bottom southeast vertex.

    Useful property:
    If the anchor point is denoted by A, then for any point
    X within the chunk, X_i >= X_i.
  */
  Vec3 anchorPoint() const;

  /*
    \returns The chunk's (local) geometric center.
  */
  Vec3 center() const { return anchorPoint() + s_ChunkLength / 2; }

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

  bool isBlockNeighborInAnotherChunk(const BlockIndex& blockIndex, BlockFace face);
  bool isBlockNeighborTransparent(blockIndex_t i, blockIndex_t j, blockIndex_t k, BlockFace face);
  bool isBlockNeighborTransparent(const BlockIndex& blockIndex, BlockFace face);
  bool isBlockNeighborAir(const BlockIndex& blockIndex, BlockFace face);

  static constexpr uint8_t Size() { return s_ChunkSize; }
  static constexpr length_t Length() { return s_ChunkLength; }
  static constexpr uint32_t TotalBlocks() { return s_ChunkTotalBlocks; }
  static const LocalIndex LocalIndexFromPos(const Vec3& position);

  static void InitializeIndexBuffer();

  static constexpr LocalIndex CalcRelativeIndex(const GlobalIndex& indexA, const GlobalIndex& indexB)
  {
    return { static_cast<localIndex_t>(indexA.i - indexB.i),
             static_cast<localIndex_t>(indexA.j - indexB.j),
             static_cast<localIndex_t>(indexA.k - indexB.k) };
  }

private:
  enum class MeshState : uint8_t
  {
    NotGenerated = 0,
    NeedsUpdate,
    Simple,
    Optimized
  };

  // Size and dimension
  static constexpr blockIndex_t s_ChunkSize = 32;
  static constexpr length_t s_ChunkLength = s_ChunkSize * Block::Length();
  static constexpr uint32_t s_ChunkTotalBlocks = s_ChunkSize * s_ChunkSize * s_ChunkSize;

  // Position and composition
  GlobalIndex m_GlobalIndex;
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

  /*
    Generates simplistic mesh based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.
  */
  void generateMesh();

  void generateVertexArray();

  void setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, BlockType blockType);
  void setBlockType(const BlockIndex& blockIndex, BlockType blockType);

  void determineOpacity();

  /*
    For updating neighbors if chunk moves to a different
    location in memory.
  */
  void sendAddressUpdate();

  /*
    Severs communication with neighboring chunks.
  */
  void excise();

  void markAsEmpty();
};