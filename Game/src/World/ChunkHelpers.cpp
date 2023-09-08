#include "GMpch.h"
#include "ChunkHelpers.h"
#include "Chunk.h"

ChunkVertex::ChunkVertex()
  : m_VertexData(0), m_Lighting(0) {}
ChunkVertex::ChunkVertex(const BlockIndex& vertexPlacement, int quadIndex, Block::Texture texture, int sunlight, int ambientOcclusion)
{
  m_VertexData =  vertexPlacement.i + (vertexPlacement.j << 6) + (vertexPlacement.k << 12);
  m_VertexData |= quadIndex << 18;
  m_VertexData |= static_cast<blockTexID>(texture) << 20;

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



ChunkQuad::ChunkQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion)
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

void ChunkDrawCommand::addQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion)
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
  // Find block index that is closest to the specified position
  BlockIndex playerBlock = BlockIndex::ToIndex(viewPosition / Block::Length());
  BlockIndex originBlock = playerBlock;
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

  // Sort based on L1 distance to originBlock
  std::sort(m_Voxels.begin(), m_Voxels.end(), [&originBlock](const ChunkVoxel& voxelA, const ChunkVoxel& voxelB)
    {
      BlockIndex diffA = voxelA.index() - originBlock;
      int distA = std::abs(diffA.i) + std::abs(diffA.j) + std::abs(diffA.k);

      BlockIndex diffB = voxelB.index() - originBlock;
      int distB = std::abs(diffB.i) + std::abs(diffB.j) + std::abs(diffB.k);

      return distA > distB;
    });
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
  m_Indices.reserve(m_IndexCount);

  Vec3 chunkAnchor = Chunk::AnchorPosition(m_ID, originIndex);

  for (const ChunkVoxel& voxel : m_Voxels)
  {
    Vec3 blockCenter = chunkAnchor + Block::Length() * Vec3(voxel.index()) + Vec3(Block::Length()) / 2;
    Vec3 toBlock = blockCenter - viewPosition;

    int quadVertexOffset = 0;
    std::array<int, 6> quadVertexOffsets{ -1, -1, -1, -1, -1, -1 };
    for (Direction face : Directions())
      if (voxel.faceEnabled(face))
      {
        quadVertexOffsets[static_cast<int>(face)] = quadVertexOffset;
        quadVertexOffset += 4;
      }

    for (Axis axis : Axes())
    {
      int axisID = static_cast<int>(axis);
      int faceID = 2 * axisID + (toBlock[axisID] < 0 ? 0 : 1);
      if (quadVertexOffsets[faceID] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[faceID]);
    }

    for (Axis axis : Axes())
    {
      int axisID = static_cast<int>(axis);
      int faceID = 2 * axisID + (toBlock[axisID] < 0 ? 1 : 0);
      if (quadVertexOffsets[faceID] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[faceID]);
    }
  }
}