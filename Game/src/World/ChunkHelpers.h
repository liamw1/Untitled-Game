#pragma once
#include "Block/Block.h"

/*
  Represents a block vertex and stores data in a compressed format.
  Compresed format is follows,

  Vertex Data:
    bits 0-17:  Relative position of vertex within chunk (3-comps, 6 bits each)
    bits 18-19: Quad index
    bits 20-31: Texure ID

  Lighting Data:
    bits 16-19: Sunlight intensity
    bits 20-22: Ambient occlusion level
*/
class ChunkVertex
{
public:
  ChunkVertex();
  ChunkVertex(const BlockIndex& vertexPlacement, i32 quadIndex, block::TextureID texture, i32 sunlight, i32 ambientOcclusion);

  static const BlockIndex& GetOffset(eng::math::Direction face, i32 quadIndex);

private:
  u32 m_VertexData;
  u32 m_LightingData;
};

/*
  Represents a block quad. Upon construction, decides where to put quad seam
  based on the lighting values at the vertices.
*/
class ChunkQuad
{
public:
  ChunkQuad(const BlockIndex& blockIndex, eng::math::Direction face, block::TextureID texture, const std::array<i32, 4>& sunlight, const std::array<i32, 4>& ambientOcclusion);

private:
  std::array<ChunkVertex, 4> m_Vertices;
};

/*
  Represents the renderable portions of a voxel. Stores as little information as
  possible, as these need to be sorted quickly at runtime.
*/
class ChunkVoxel
{
public:
  ChunkVoxel(const BlockIndex& blockIndex, eng::math::DirectionBitMask enabledFaces, i32 firstVertex);

  const BlockIndex& index() const;
  bool faceEnabled(eng::math::Direction direction) const;
  i32 baseVertex() const;

private:
  BlockIndex m_Index;
  eng::math::DirectionBitMask m_EnabledFaces;
  i32 m_BaseVertex;
};

class ChunkDrawCommand : public eng::MultiDrawIndexedCommand<GlobalIndex, ChunkDrawCommand>
{
public:
  ChunkDrawCommand(const GlobalIndex& chunkIndex, bool needsSorting);

  ChunkDrawCommand(ChunkDrawCommand&& other) noexcept;
  ChunkDrawCommand& operator=(ChunkDrawCommand&& other) noexcept;

  bool operator==(const ChunkDrawCommand& other) const;

  i32 vertexCount() const;

  const void* indexData();
  const void* vertexData();
  void prune();

  void addQuad(const BlockIndex& blockIndex, eng::math::Direction face, block::TextureID texture, const std::array<i32, 4>& sunlight, const std::array<i32, 4>& ambientOcclusion);
  void addVoxel(const BlockIndex& blockIndex, eng::math::DirectionBitMask enabledFaces);

  /*
    Sorts indices so that triangles will be rendered from back to front.
    The sorting algorithm used is O(n + k), where n is the number of voxels
    and k is the maximum L1 distance that two blocks can be within a chunk.
  */
  bool sort(const GlobalIndex& originIndex, const eng::math::Vec3& viewPosition);

private:
  std::vector<ChunkQuad> m_Quads;
  std::vector<ChunkVoxel> m_Voxels;
  std::vector<u32> m_Indices;
  BlockIndex m_SortState;
  bool m_NeedsSorting;

  i32 m_VoxelBaseVertex;

  void addQuadIndices(i32 baseVertex);
  void reorderIndices(const GlobalIndex& originIndex, const eng::math::Vec3& viewPosition);
};

namespace std
{
  template<>
  struct hash<ChunkDrawCommand>
  {
    size_t operator()(const ChunkDrawCommand& drawCommand) const
    {
      return std::hash<GlobalIndex>()(drawCommand.id());
    }
  };
}