#pragma once
#include "Block/Block.h"
#include "Indexing.h"
#include "Util/Noise.h"
#include "Util/MultiDimArrays.h"

struct HeightMap
{
  globalIndex_t chunkI;
  globalIndex_t chunkJ;
  HeapArray2D<Noise::SurfaceData, 32> surfaceData;  // NOTE: Chunk size is hard-coded here, should fix this

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

  Block::Type getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const;
  Block::Type getBlockType(const BlockIndex& blockIndex) const;
  Block::Type getBlockType(const Vec3& position) const;

  void removeBlock(const BlockIndex& blockIndex);
  void placeBlock(const BlockIndex& blockIndex, Block::Face face, Block::Type blockType);

  /*
    \returns The local block index of the block that the given position
             sits inside.  Assumes that given position is inside chunk.
  */
  BlockIndex blockIndexFromPos(const Vec3& position) const;

  /*
    Creates and fills composition array with blocks based on the given height map.
    Deletes composition array if all blocks are air.
  */
  void load(const HeightMap& heightMap);

  /*
    Applies necessary updates to chunk.  
    Chunk may be re-meshed, so call conservatively.
  */
  void update();

  void draw();

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
    A chunk's anchor point is its bottom southeast vertex.
    Position given relative to the anchor of the origin chunk.

    Useful property:
    If the anchor point is denoted by A, then for any point
    X within the chunk, X_i >= A_i.
  */
  Vec3 anchorPosition() const { return s_ChunkLength * static_cast<Vec3>(getLocalIndex()); }

  /*
    \returns The chunk's geometric center relative to origin chunk.
  */
  Vec3 center() const { return anchorPosition() + s_ChunkLength / 2; }

  /*
    \returns A pointer to the neighboring chunk in the specified direction.
             Will return nullptr if neighbor has not been loaded yet. 
    
    Neighbor is allowed to be modified, use this power responsibly!
  */
  Chunk* getNeighbor(Block::Face face) const { return m_Neighbors[static_cast<int>(face)]; }

  /*
    Sets neighboring chunk in the specified direction.
    Upon use, a flag is set to re-mesh chunk when appropriate.
  */
  void setNeighbor(Block::Face face, Chunk* chunk);

  int getQuadCount() const { return m_QuadCount; }
  const Unique<Engine::VertexArray>& getVertexArray() const { return m_VertexArray; }

  bool isEmpty() const { return m_ChunkComposition == nullptr; }
  bool isFaceOpaque(Block::Face face) const { return !(m_NonOpaqueFaces & bit(static_cast<int>(face))); }

  bool isBlockNeighborInAnotherChunk(const BlockIndex& blockIndex, Block::Face face);
  bool isBlockNeighborTransparent(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Face face);
  bool isBlockNeighborTransparent(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Face faceA, Block::Face faceB);
  bool isBlockNeighborTransparent(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Face faceA, Block::Face faceB, Block::Face faceC);
  bool isBlockNeighborTransparent(const BlockIndex& blockIndex, Block::Face face);
  bool isBlockNeighborTransparent(const BlockIndex& blockIndex, Block::Face faceA, Block::Face faceB);
  bool isBlockNeighborTransparent(const BlockIndex& blockIndex, Block::Face faceA, Block::Face faceB, Block::Face faceC);
  bool isBlockNeighborAir(const BlockIndex& blockIndex, Block::Face face);

  Block::Type getBlockNeighbor(const BlockIndex& blockIndex, Block::Face face);

  static constexpr blockIndex_t Size() { return s_ChunkSize; }
  static constexpr length_t Length() { return s_ChunkLength; }
  static constexpr uint32_t TotalBlocks() { return s_ChunkTotalBlocks; }
  static LocalIndex LocalIndexFromPos(const Vec3& position);

  static void BindBuffers();

  static void Initialize(const Shared<Engine::TextureArray>& textureArray);

private:
  enum class MeshState : uint8_t
  {
    NotGenerated = 0,
    NeedsUpdate,
    Simple,
    Optimized
  };

  struct Uniforms
  {
    Float3 anchorPosition;
    const float blockLength = static_cast<float>(Block::Length());
  };

  // Size and dimension
  static constexpr blockIndex_t s_ChunkSize = 32;
  static constexpr length_t s_ChunkLength = s_ChunkSize * Block::Length();
  static constexpr uint32_t s_ChunkTotalBlocks = s_ChunkSize * s_ChunkSize * s_ChunkSize;

  // Position and composition
  GlobalIndex m_GlobalIndex;
  Unique<Block::Type[]> m_ChunkComposition = nullptr;
  uint8_t m_NonOpaqueFaces = 0;

  // Mesh data
  MeshState m_MeshState = MeshState::NotGenerated;
  uint16_t m_QuadCount;
  Unique<Engine::VertexArray> m_VertexArray;
  static Unique<Engine::Shader> s_Shader;
  static Shared<Engine::TextureArray> s_TextureArray;
  static Unique<Engine::UniformBuffer> s_UniformBuffer;
  static Shared<const Engine::IndexBuffer> s_IndexBuffer;
  static const Engine::BufferLayout s_VertexBufferLayout;
  static constexpr int s_TextureSlot = 0;

  // Chunk neighbor data
  std::array<Chunk*, 6> m_Neighbors{};

  /*
    Generates simplistic mesh in a compressed format based on chunk compostion.
    Block faces covered by opaque blocks will not be added to mesh.

    Compresed format is follows,
     bits 0-17:  Relative position of vertex within chunk (3-comps, 6 bits each)
     bits 18-20: Normal direction of quad (follows BlockFace convention)
     bits 21-22: Texture coordinate index (see BlockFace.glsl for details)
     bits 23-31: Texure ID
  */
  void generateMesh();

  void setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType);
  void setBlockType(const BlockIndex& blockIndex, Block::Type blockType);

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