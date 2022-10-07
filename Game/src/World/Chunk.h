#pragma once
#include "Indexing.h"
#include "Util/Noise.h"
#include "Util/MultiDimArrays.h"
#include <Engine.h>

struct HeightMap;

class Chunk
{
public:
  Chunk();
  Chunk(const GlobalIndex& chunkIndex);
  ~Chunk();

  Chunk(Chunk&& other) noexcept;
  Chunk& operator=(Chunk&& other) noexcept;

  // Chunks are unique and so cannot be copied
  Chunk(const Chunk& other) = delete;
  Chunk& operator=(const Chunk& other) = delete;

  /*
    \returns The chunk's global index, which identifies it uniquely.
  */
  const GlobalIndex& getGlobalIndex() const { return m_GlobalIndex; }

  /*
    \returns The chunk's index relative to the origin chunk.
  */
  LocalIndex getLocalIndex() const;

  /*
    A chunk's anchor point is its bottom southeast vertex.
    Position given relative to the anchor of the origin chunk.
    Useful property:
    If the anchor point is denoted by A, then for any point
    X within the chunk, X_i >= A_i.
  */
  Vec3 anchorPosition() const { return Chunk::Length() * static_cast<Vec3>(getLocalIndex()); }

  /*
    \returns The chunk's geometric center relative to origin chunk.
  */
  Vec3 center() const { return anchorPosition() + Chunk::Length() / 2; }

  bool isEmpty() const { return m_Composition == nullptr; }
  bool isFaceOpaque(Block::Face face) const { return !(m_NonOpaqueFaces & bit(static_cast<int>(face))); }

  Block::Type getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const;
  Block::Type getBlockType(const BlockIndex& blockIndex) const;

  uint32_t getQuadCount() const { return m_QuadCount; }

  static constexpr blockIndex_t Size() { return s_ChunkSize; }
  static constexpr length_t Length() { return Block::Length() * s_ChunkSize; }
  static constexpr int TotalBlocks() { return s_ChunkSize * s_ChunkSize * s_ChunkSize; }

  static void Initialize(const Shared<Engine::TextureArray>& textureArray);
  static void BindBuffers();

private:
  struct Uniforms
  {
    Float3 anchorPosition;
  };

  static constexpr int s_ChunkSize = 32;

  // Mesh Data
  static inline Unique<Engine::Shader> s_Shader;
  static inline Shared<Engine::TextureArray> s_TextureArray;
  static inline Shared<const Engine::IndexBuffer> s_IndexBuffer;
  static inline const Engine::BufferLayout s_VertexBufferLayout = { { ShaderDataType::Uint32, "a_VertexData" } };
  static constexpr int s_TextureSlot = 0;
  static constexpr int s_UniformBinding = 2;

  mutable std::mutex m_Mutex;
  std::vector<uint32_t> m_Mesh;
  Unique<Engine::VertexArray> m_VertexArray;
  Block::Type* m_Composition;
  GlobalIndex m_GlobalIndex;
  uint16_t m_NonOpaqueFaces;
  uint16_t m_QuadCount;

  void setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType);
  void setBlockType(const BlockIndex& blockIndex, Block::Type blockType);

  void setData(Block::Type* composition);
  void setMesh(const uint32_t* meshData, uint16_t quadCount);
  void determineOpacity();

  void update();
  void draw() const;
  void draw2(); // TODO: Remove
  void clear();
  void reset();

  std::lock_guard<std::mutex> acquireLock() const { return std::lock_guard(m_Mutex); };

  friend class ChunkManager;
  friend class MTChunkManager; // Remove
  friend class ChunkFiller;
};