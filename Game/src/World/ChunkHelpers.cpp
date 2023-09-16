#include "GMpch.h"
#include "ChunkHelpers.h"
#include "Chunk.h"

ChunkVertex::ChunkVertex()
  : m_VertexData(0), m_Lighting(0) {}
ChunkVertex::ChunkVertex(const BlockIndex& vertexPlacement, int quadIndex, Block::TextureID texture, int sunlight, int ambientOcclusion)
{
  m_VertexData =  vertexPlacement.i + (vertexPlacement.j << 6) + (vertexPlacement.k << 12);
  m_VertexData |= quadIndex << 18;
  m_VertexData |= std::underlying_type_t<Block::TextureID>(texture) << 20;

  m_Lighting =  sunlight << 16;
  m_Lighting |= ambientOcclusion << 20;
}

const BlockIndex& ChunkVertex::GetOffset(Direction face, int quadIndex)
{
  static constexpr BlockIndex offsets[6][4]
    = { { {0, 1, 0}, {0, 0, 0}, {0, 1, 1}, {0, 0, 1} },    /*  West Face   */
        { {1, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 1, 1} },    /*  East Face   */
        { {0, 0, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0, 1} },    /*  South Face  */
        { {1, 1, 0}, {0, 1, 0}, {1, 1, 1}, {0, 1, 1} },    /*  North Face  */
        { {0, 1, 0}, {1, 1, 0}, {0, 0, 0}, {1, 0, 0} },    /*  Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1} } };  /*  Top Face    */

  return offsets[static_cast<int>(face)][quadIndex];
}



ChunkQuad::ChunkQuad(const BlockIndex& blockIndex, Direction face, Block::TextureID texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion)
{
  static constexpr std::array<int, 4> standardOrder = { 0, 1, 2, 3 };
  static constexpr std::array<int, 4> reversedOrder = { 1, 3, 0, 2 };

  auto totalLightAtVertex = [&sunlight, &ambientOcclusion](int index)
  {
    return sunlight[index] + ambientOcclusion[index];
  };

  int lightDifferenceAlongStandardSeam = std::abs(totalLightAtVertex(2) - totalLightAtVertex(1));
  int lightDifferenceAlongReversedSeam = std::abs(totalLightAtVertex(3) - totalLightAtVertex(0));
  const std::array<int, 4>& quadOrder = lightDifferenceAlongStandardSeam > lightDifferenceAlongReversedSeam ? reversedOrder : standardOrder;
  for (int i = 0; i < 4; ++i)
  {
    int quadIndex = quadOrder[i];
    BlockIndex vertexPlacement = blockIndex + ChunkVertex::GetOffset(face, quadIndex);
    m_Vertices[i] = ChunkVertex(vertexPlacement, quadIndex, texture, sunlight[quadIndex], ambientOcclusion[quadIndex]);
  }
}



ChunkVoxel::ChunkVoxel(const BlockIndex& blockIndex, uint8_t enabledFaces, int firstVertex)
  : m_Index(blockIndex),
    m_EnabledFaces(enabledFaces),
    m_BaseVertex(firstVertex) {}

const BlockIndex& ChunkVoxel::index() const
{
  return m_Index;
}

bool ChunkVoxel::faceEnabled(Direction direction) const
{
  return (m_EnabledFaces >> static_cast<int>(direction)) & 0x1;
}

int ChunkVoxel::baseVertex() const
{
  return m_BaseVertex;
}



ChunkDrawCommand::ChunkDrawCommand(const GlobalIndex& chunkIndex, bool needsSorting)
  : Engine::MultiDrawIndexedCommand<GlobalIndex, ChunkDrawCommand>(chunkIndex, 0),
    m_SortState(-1, -1, -1),
    m_NeedsSorting(needsSorting),
    m_VoxelBaseVertex(0) {}

ChunkDrawCommand::ChunkDrawCommand(ChunkDrawCommand&& other) noexcept = default;
ChunkDrawCommand& ChunkDrawCommand::operator=(ChunkDrawCommand&& other) noexcept = default;

bool ChunkDrawCommand::operator==(const ChunkDrawCommand& other) const
{
  return m_ID == other.m_ID;
}

int ChunkDrawCommand::vertexCount() const
{
  return static_cast<int>(4 * m_Quads.size());
}

const void* ChunkDrawCommand::indexData()
{
  return m_Indices.data();
}

const void* ChunkDrawCommand::vertexData()
{
  return m_Quads.data();
}

void ChunkDrawCommand::prune()
{
  m_Quads.clear();
  m_Quads.shrink_to_fit();
  if (!m_NeedsSorting)
  {
    m_Voxels = {};
    m_Indices = {};
  }
}

void ChunkDrawCommand::addQuad(const BlockIndex& blockIndex, Direction face, Block::TextureID texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion)
{
  addQuadIndices(vertexCount());
  m_IndexCount += 6;
  m_Quads.emplace_back(blockIndex, face, texture, sunlight, ambientOcclusion);
}

void ChunkDrawCommand::addVoxel(const BlockIndex& blockIndex, uint8_t enabledFaces)
{
  m_Voxels.emplace_back(blockIndex, enabledFaces, m_VoxelBaseVertex);
  m_VoxelBaseVertex = vertexCount();
}

bool ChunkDrawCommand::sort(const GlobalIndex& originIndex, const Vec3& viewPosition)
{
  static constexpr int c_MaxL1Distance = 3 * (Chunk::Size() - 1);

  // Find block index that is closest to the specified position
  BlockIndex originBlock = BlockIndex::ToIndex(viewPosition / Block::Length());
  EN_ASSERT(Chunk::Bounds().encloses(originBlock), "Given view position is not inside the origin chunk!");
  for (Axis axis : Axes())
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
  std::array<int, c_MaxL1Distance + 1> counts{};
  for (ChunkVoxel voxel : m_Voxels)
  {
    int key = c_MaxL1Distance - (voxel.index() - originBlock).l1Norm();
    counts[key]++;
  }
  std::partial_sum(counts.begin(), counts.end(), counts.begin());

  std::array<int, c_MaxL1Distance + 1> placements = counts;
  for (size_t i = 0; i < m_Voxels.size();)
  {
    int key = c_MaxL1Distance - (m_Voxels[i].index() - originBlock).l1Norm();
    int prevCount = key > 0 ? counts[key - 1] : 0;

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

void ChunkDrawCommand::addQuadIndices(int baseVertex)
{
  m_Indices.push_back(baseVertex + 0);
  m_Indices.push_back(baseVertex + 1);
  m_Indices.push_back(baseVertex + 2);

  m_Indices.push_back(baseVertex + 1);
  m_Indices.push_back(baseVertex + 3);
  m_Indices.push_back(baseVertex + 2);
}

void ChunkDrawCommand::reorderIndices(const GlobalIndex& originIndex, const Vec3& viewPosition)
{
  m_Indices.clear();

  Vec3 chunkAnchor = Chunk::AnchorPosition(m_ID, originIndex);
  for (ChunkVoxel voxel : m_Voxels)
  {
    Vec3 blockCenter = chunkAnchor + Block::Length() * Vec3(voxel.index()) + Vec3(Block::Length()) / 2;
    Vec3 toBlock = blockCenter - viewPosition;

    int quadVertexOffset = 0;
    DirectionArray<int> quadVertexOffsets(-1);
    for (Direction face : Directions())
      if (voxel.faceEnabled(face))
      {
        quadVertexOffsets[face] = quadVertexOffset;
        quadVertexOffset += 4;
      }

    // Add back-facing quads
    for (Axis axis : Axes())
    {
      Direction face = ToDirection(axis, toBlock[static_cast<int>(axis)] > 0);
      if (quadVertexOffsets[face] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[face]);
    }

    // Add front-facing quads
    for (Axis axis : Axes())
    {
      Direction face = ToDirection(axis, toBlock[static_cast<int>(axis)] <= 0);
      if (quadVertexOffsets[face] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[face]);
    }
  }
}