#include "GMpch.h"
#include "ChunkHelpers.h"
#include "Chunk.h"
#include "Indexing/Operations.h"

ChunkVertex::ChunkVertex()
  : m_VertexData(0), m_LightingData(0) {}
ChunkVertex::ChunkVertex(const BlockIndex& vertexPlacement, i32 quadIndex, block::TextureID texture, i32 sunlight, i32 ambientOcclusion)
{
  m_VertexData =  vertexPlacement.i + (vertexPlacement.j << 6) + (vertexPlacement.k << 12);
  m_VertexData |= quadIndex << 18;
  m_VertexData |= std::underlying_type_t<block::TextureID>(texture) << 20;

  m_LightingData =  sunlight << 16;
  m_LightingData |= ambientOcclusion << 20;
}

const BlockIndex& ChunkVertex::GetOffset(eng::math::Direction face, i32 quadIndex)
{
  static constexpr eng::EnumArray<std::array<BlockIndex, 4>, eng::math::Direction> offsets =
  { { eng::math::Direction::West,   { { {0, 1, 0}, {0, 0, 0}, {0, 1, 1}, {0, 0, 1} } } },
    { eng::math::Direction::East,   { { {1, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 1, 1} } } },
    { eng::math::Direction::South,  { { {0, 0, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0, 1} } } },
    { eng::math::Direction::North,  { { {1, 1, 0}, {0, 1, 0}, {1, 1, 1}, {0, 1, 1} } } },
    { eng::math::Direction::Bottom, { { {0, 1, 0}, {1, 1, 0}, {0, 0, 0}, {1, 0, 0} } } },
    { eng::math::Direction::Top,    { { {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1} } } } };

  return offsets[face][quadIndex];
}



ChunkVoxel::ChunkVoxel(const BlockIndex& blockIndex, eng::EnumBitMask<eng::math::Direction> enabledFaces, i32 firstVertex)
  : m_Index(blockIndex),
    m_EnabledFaces(enabledFaces),
    m_BaseVertex(firstVertex) {}

const BlockIndex& ChunkVoxel::index() const
{
  return m_Index;
}

bool ChunkVoxel::faceEnabled(eng::math::Direction direction) const
{
  return m_EnabledFaces[direction];
}

i32 ChunkVoxel::baseVertex() const
{
  return m_BaseVertex;
}



ChunkDrawCommand::ChunkDrawCommand(const GlobalIndex& chunkIndex, bool needsSorting)
  : eng::IndexedDrawCommand<ChunkDrawCommand, GlobalIndex>(chunkIndex),
    m_SortState(-1, -1, -1),
    m_NeedsSorting(needsSorting),
    m_VoxelBaseVertex(0) {}

bool ChunkDrawCommand::operator==(const ChunkDrawCommand& other) const { return id() == other.id(); }

eng::mem::IndexData ChunkDrawCommand::indexData() const { return m_Indices; }
eng::mem::Data ChunkDrawCommand::vertexData() const { return m_Vertices; }

void ChunkDrawCommand::clearData()
{
  m_Vertices = {};
  if (!m_NeedsSorting)
  {
    m_Voxels = {};
    m_Indices = {};
  }
}

void ChunkDrawCommand::addQuad(const BlockIndex& blockIndex, eng::math::Direction face, block::TextureID texture, const std::array<i32, 4>& sunlight, const std::array<i32, 4>& ambientOcclusion)
{
  static constexpr std::array<i32, 4> standardOrder = { 0, 1, 2, 3 };
  static constexpr std::array<i32, 4> reversedOrder = { 1, 3, 0, 2 };
  auto totalLightAtVertex = [&sunlight, &ambientOcclusion](i32 index) { return sunlight[index] + ambientOcclusion[index]; };

  i32 lightDifferenceAlongStandardSeam = std::abs(totalLightAtVertex(2) - totalLightAtVertex(1));
  i32 lightDifferenceAlongReversedSeam = std::abs(totalLightAtVertex(3) - totalLightAtVertex(0));
  const std::array<i32, 4>& quadOrder = lightDifferenceAlongStandardSeam > lightDifferenceAlongReversedSeam ? reversedOrder : standardOrder;
  for (i32 i = 0; i < 4; ++i)
  {
    i32 quadIndex = quadOrder[i];
    BlockIndex vertexPlacement = blockIndex + ChunkVertex::GetOffset(face, quadIndex);
    m_Vertices.emplace_back(vertexPlacement, quadIndex, texture, sunlight[quadIndex], ambientOcclusion[quadIndex]);
  }
  addQuadIndices(eng::arithmeticCast<i32>(m_Vertices.size() - 4));
}

void ChunkDrawCommand::addVoxel(const BlockIndex& blockIndex, eng::EnumBitMask<eng::math::Direction> enabledFaces)
{
  m_Voxels.emplace_back(blockIndex, enabledFaces, m_VoxelBaseVertex);
  m_VoxelBaseVertex = eng::arithmeticCast<i32>(m_Vertices.size());
}

bool ChunkDrawCommand::sort(const GlobalIndex& originIndex, const eng::math::Vec3& viewPosition)
{
  using keyType = std::make_unsigned_t<blockIndex_t>;
  static constexpr keyType c_MaxL1Distance = 3 * (Chunk::Size() - 1);

  // Find block index that is closest to the specified position
  BlockIndex originBlock = BlockIndex::ToIndex(viewPosition / block::length());
  ENG_ASSERT(Chunk::Bounds().encloses(originBlock), "Given view position is not inside the origin chunk!");
  for (eng::math::Axis axis : eng::math::Axes())
  {
    if (originIndex[axis] > id()[axis])
      originBlock[axis] = Chunk::Size() - 1;
    else if (originIndex[axis] < id()[axis])
      originBlock[axis] = 0;
  }

  // If this block index is the same as the previous sort, no need to sort
  if (originBlock == m_SortState)
    return false;

  // Perform an in-place counting sort on L1 distance to originBlock, from highest to lowest
  std::array<i16, c_MaxL1Distance + 1> counts{};
  for (ChunkVoxel voxel : m_Voxels)
  {
    keyType key = c_MaxL1Distance - (voxel.index() - originBlock).l1Norm();
    counts[key]++;
  }
  eng::algo::partialSum(counts, counts.begin());

  std::array<i16, c_MaxL1Distance + 1> placements = counts;
  for (uSize i = 0; i < m_Voxels.size();)
  {
    keyType key = c_MaxL1Distance - (m_Voxels[i].index() - originBlock).l1Norm();
    keyType prevCount = key > 0 ? counts[key - 1] : 0;

    if (prevCount <= i && i < counts[key])
      i++;
    else
    {
      placements[key]--;
      std::swap(m_Voxels[i], m_Voxels[placements[key]]);
    }
  }

  // Update sort state and indices
  m_SortState = originBlock;
  reorderIndices(originIndex, viewPosition);

  return true;
}

void ChunkDrawCommand::addQuadIndices(i32 baseVertex)
{
  m_Indices.push_back(baseVertex + 0);
  m_Indices.push_back(baseVertex + 1);
  m_Indices.push_back(baseVertex + 2);

  m_Indices.push_back(baseVertex + 1);
  m_Indices.push_back(baseVertex + 3);
  m_Indices.push_back(baseVertex + 2);
}

void ChunkDrawCommand::reorderIndices(const GlobalIndex& originIndex, const eng::math::Vec3& viewPosition)
{
  m_Indices.clear();

  eng::math::Vec3 chunkAnchorPosition = indexPosition(id(), originIndex);
  for (ChunkVoxel voxel : m_Voxels)
  {
    eng::math::Vec3 blockCenter = chunkAnchorPosition + block::length() * eng::math::Vec3(voxel.index()) + eng::math::Vec3(block::length()) / 2;
    eng::math::Vec3 toBlock = blockCenter - viewPosition;

    i32 quadVertexOffset = 0;
    eng::EnumArray<i32, eng::math::Direction> quadVertexOffsets(-1);
    for (eng::math::Direction face : eng::math::Directions())
      if (voxel.faceEnabled(face))
      {
        quadVertexOffsets[face] = quadVertexOffset;
        quadVertexOffset += 4;
      }

    // Add back-facing quads
    for (eng::math::Axis axis : eng::math::Axes())
    {
      eng::math::Direction face = toDirection(axis, toBlock[eng::enumIndex(axis)] > 0);
      if (quadVertexOffsets[face] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[face]);
    }

    // Add front-facing quads
    for (eng::math::Axis axis : eng::math::Axes())
    {
      eng::math::Direction face = toDirection(axis, toBlock[eng::enumIndex(axis)] <= 0);
      if (quadVertexOffsets[face] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[face]);
    }
  }
}