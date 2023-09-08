#pragma once
#include "Block/Block.h"

class ChunkVertex
{
public:
  ChunkVertex();
  ChunkVertex(const BlockIndex& vertexPlacement, int quadIndex, Block::Texture texture, int sunlight, int ambientOcclusion);

  static const BlockIndex& GetOffset(Direction face, int quadIndex);

private:
  uint32_t m_VertexData;
  uint32_t m_Lighting;
};

class ChunkQuad
{
public:
  ChunkQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion);

private:
  std::array<ChunkVertex, 4> m_Vertices;
};

class ChunkVoxel
{
public:
  ChunkVoxel(const BlockIndex& blockIndex, uint8_t enabledFaces, int firstVertex);

  const BlockIndex& index() const;
  bool faceEnabled(Direction direction) const;
  int baseVertex() const;

private:
  BlockIndex m_Index;
  uint8_t m_EnabledFaces;
  int m_BaseVertex;
};

class ChunkDrawCommand : public Engine::MultiDrawIndexedCommand<GlobalIndex, ChunkDrawCommand>, private Engine::NonCopyable
{
public:
  ChunkDrawCommand(const GlobalIndex& chunkIndex, bool needsSorting);

  ChunkDrawCommand(ChunkDrawCommand&& other) noexcept;
  ChunkDrawCommand& operator=(ChunkDrawCommand&& other) noexcept;

  bool operator==(const ChunkDrawCommand& other) const;

  int vertexCount() const;

  const void* indexData();
  const void* vertexData();
  void prune();

  void addQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion);
  void addVoxel(const BlockIndex& blockIndex, uint8_t enabledFaces);

  bool sort(const GlobalIndex& originIndex, const Vec3& viewPosition);

private:
  std::vector<ChunkQuad> m_Quads;
  std::vector<ChunkVoxel> m_Voxels;
  std::vector<uint32_t> m_Indices;
  BlockIndex m_SortState;
  bool m_NeedsSorting;

  int m_VoxelBaseVertex;

  void addQuadIndices(int baseVertex);
  void reorderIndices(const GlobalIndex& originIndex, const Vec3& viewPosition);
};

namespace std
{
  template<>
  struct hash<ChunkDrawCommand>
  {
    int operator()(const ChunkDrawCommand& drawCommand) const
    {
      return std::hash<GlobalIndex>()(drawCommand.id());
    }
  };
}