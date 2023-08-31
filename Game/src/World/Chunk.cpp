#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"
#include <iostream>

Chunk::Chunk() = default;

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(),
    m_Lighting(),
    m_NonOpaqueFaces(0x3F) {}

CubicArray<Block::Type, Chunk::Size()>& Chunk::composition()
{
  return const_cast<CubicArray<Block::Type, Chunk::Size()>&>(static_cast<const Chunk*>(this)->composition());
}

const CubicArray<Block::Type, Chunk::Size()>& Chunk::composition() const
{
  return m_Composition;
}

CubicArray<Block::Light, Chunk::Size()>& Chunk::lighting()
{
  return const_cast<CubicArray<Block::Light, Chunk::Size()>&>(static_cast<const Chunk*>(this)->lighting());
}

const CubicArray<Block::Light, Chunk::Size()>& Chunk::lighting() const
{
  return m_Lighting;
}

bool Chunk::isFaceOpaque(Direction face) const
{
  uint16_t nonOpaqueFaces = m_NonOpaqueFaces.load();
  return !(nonOpaqueFaces & Engine::Bit(static_cast<int>(face)));
}

Block::Type Chunk::getBlockType(const BlockIndex& blockIndex) const
{
  return m_Composition ? m_Composition(blockIndex) : Block::Type::Air;
}

Block::Light Chunk::getBlockLight(const BlockIndex& blockIndex) const
{
  return m_Lighting ? m_Lighting(blockIndex) : Block::Light::MaxValue();
}

void Chunk::setBlockType(const BlockIndex& blockIndex, Block::Type blockType)
{
  if (!m_Composition)
  {
    if (blockType == Block::Type::Air)
      return;

    m_Composition = CubicArray<Block::Type, c_ChunkSize>(Block::Type::Air);
  }
  m_Composition(blockIndex) = blockType;
}

void Chunk::setBlockLight(const BlockIndex& blockIndex, Block::Light blockLight)
{
  if (!m_Lighting)
  {
    if (blockLight.sunlight() == Block::Light::MaxValue())
      return;

    m_Lighting = CubicArray<Block::Light, c_ChunkSize>(Block::Light::MaxValue());
  }
  m_Lighting(blockIndex) = blockLight;
}

void Chunk::setComposition(CubicArray<Block::Type, c_ChunkSize>&& composition)
{
  if (m_Composition)
    EN_WARN("Calling setComposition on a non-empty chunk!  Deleting previous allocation...");
  m_Composition = std::move(composition);
  determineOpacity();
}

void Chunk::setLighting(CubicArray<Block::Light, c_ChunkSize>&& lighting)
{
  m_Lighting = std::move(lighting);
}

void Chunk::determineOpacity()
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  if (!m_Composition)
  {
    m_NonOpaqueFaces.store(0x3F);
    return;
  }

  uint16_t nonOpaqueFaces = 0;
  for (Direction face : Directions())
  {
    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        BlockIndex blockIndex = BlockIndex::CreatePermuted(chunkLimits[IsUpstream(face)], i, j, GetCoordID(face));
        if (Block::HasTransparency(m_Composition(blockIndex)))
        {
          nonOpaqueFaces |= Engine::Bit(static_cast<int>(face));
          goto nextFace;
        }
      }
  nextFace:;
  }
  m_NonOpaqueFaces.store(nonOpaqueFaces);
}

void Chunk::update()
{
  if (m_Composition && m_Composition.filledWith(Block::Type::Air))
    m_Composition.reset();
  if (m_Lighting && m_Lighting.filledWith(Block::Light::MaxValue()))
    m_Lighting.reset();

  determineOpacity();
}

_Acquires_lock_(return) std::unique_lock<std::mutex> Chunk::acquireLock() const
{
  return std::unique_lock(m_Mutex);
}

_Acquires_lock_(return) std::lock_guard<std::mutex> Chunk::acquireLockGuard() const
{
  return std::lock_guard(m_Mutex);
}

Vec3 Chunk::Center(const Vec3& anchorPosition)
{
  return anchorPosition + Chunk::Length() / 2;
}

Vec3 Chunk::AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex)
{
  return Chunk::Length() * static_cast<Vec3>(chunkIndex - originIndex);
}



Chunk::Vertex::Vertex()
  : m_VertexData(0), m_Lighting(0) {}
Chunk::Vertex::Vertex(const BlockIndex& vertexPlacement, int quadIndex, Block::Texture texture, int sunlight, int ambientOcclusion)
{
  m_VertexData =  vertexPlacement.i + (vertexPlacement.j << 6) + (vertexPlacement.k << 12);
  m_VertexData |= quadIndex << 18;
  m_VertexData |= static_cast<blockTexID>(texture) << 20;

  m_Lighting =  sunlight << 16;
  m_Lighting |= ambientOcclusion << 20;
}

const BlockIndex& Chunk::Vertex::GetOffset(Direction face, int quadIndex)
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



Chunk::Quad::Quad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion)
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
    BlockIndex vertexPlacement = blockIndex + Vertex::GetOffset(face, quadIndex);
    m_Vertices[i] = Vertex(vertexPlacement, quadIndex, texture, sunlight[quadIndex], ambientOcclusion[quadIndex]);
  }
}



Chunk::Voxel::Voxel(const BlockIndex& blockIndex, uint8_t enabledFaces, int firstVertex)
  : m_Index(blockIndex),
    m_EnabledFaces(enabledFaces),
    m_BaseVertex(firstVertex) {}

const BlockIndex& Chunk::Voxel::index() const
{
  return m_Index;
}

bool Chunk::Voxel::faceEnabled(Direction direction) const
{
  return (m_EnabledFaces >> static_cast<int>(direction)) & 0x1;
}

int Chunk::Voxel::baseVertex() const
{
  return m_BaseVertex;
}



Chunk::DrawCommand::DrawCommand(const GlobalIndex& chunkIndex, bool canPruneIndices)
  : Engine::MultiDrawIndexedCommand<GlobalIndex, DrawCommand>(chunkIndex, 0),
    m_SortState(-1, -1, -1),
    m_CanPruneIndices(canPruneIndices),
    m_VoxelBaseVertex(0) {}

Chunk::DrawCommand::DrawCommand(DrawCommand&& other) noexcept = default;
Chunk::DrawCommand& Chunk::DrawCommand::operator=(DrawCommand&& other) noexcept = default;

bool Chunk::DrawCommand::operator==(const DrawCommand& other) const
{
  return m_ID == other.m_ID;
}

int Chunk::DrawCommand::vertexCount() const
{
  return static_cast<int>(4 * m_Quads.size());
}

const void* Chunk::DrawCommand::indexData()
{
  return m_Indices.data();
}

const void* Chunk::DrawCommand::vertexData()
{
  return m_Quads.data();
}

void Chunk::DrawCommand::prune()
{
  m_Quads.clear();
  m_Quads.shrink_to_fit();
  if (m_CanPruneIndices)
  {
    m_Voxels.clear();
    m_Voxels.shrink_to_fit();
    m_Indices.clear();
    m_Indices.shrink_to_fit();
  }
}

void Chunk::DrawCommand::addQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, const std::array<int, 4>& sunlight, const std::array<int, 4>& ambientOcclusion)
{
  m_Quads.emplace_back(blockIndex, face, texture, sunlight, ambientOcclusion);
  m_IndexCount += 6;
}

void Chunk::DrawCommand::addVoxel(const BlockIndex& blockIndex, uint8_t enabledFaces)
{
  m_Voxels.emplace_back(blockIndex, enabledFaces, m_VoxelBaseVertex);
  m_VoxelBaseVertex = static_cast<int>(4 * m_Quads.size());
}

bool Chunk::DrawCommand::sort(const GlobalIndex& originIndex, const Vec3& viewPosition)
{
  // Find block index that is closest to the specified position
  BlockIndex playerBlock = BlockIndex::ToIndex(viewPosition / Block::Length());
  BlockIndex originBlock = playerBlock;
  for (int i = 0; i < 3; ++i)
  {
    if (originIndex[i] > id()[i])
      originBlock[i] = Chunk::Size() - 1;
    else if (originIndex[i] < id()[i])
      originBlock[i] = 0;
  }

  // If this block index is the same as the previous sort, no need to sort
  if (originBlock == m_SortState)
    return false;

  // Sort based on L1 distance to originBlock
  std::sort(m_Voxels.begin(), m_Voxels.end(), [&originBlock](const Voxel& voxelA, const Voxel& voxelB)
    {
      BlockIndex diffA = voxelA.index() - originBlock;
      int distA = std::abs(diffA.i) + std::abs(diffA.j) + std::abs(diffA.k);

      BlockIndex diffB = voxelB.index() - originBlock;
      int distB = std::abs(diffB.i) + std::abs(diffB.j) + std::abs(diffB.k);

      return distA > distB;
    });
  m_SortState = originBlock;

  setIndices(originIndex, viewPosition);
  return true;
}

void Chunk::DrawCommand::addQuadIndices(int baseVertex)
{
  m_Indices.push_back(baseVertex + 0);
  m_Indices.push_back(baseVertex + 1);
  m_Indices.push_back(baseVertex + 2);

  m_Indices.push_back(baseVertex + 1);
  m_Indices.push_back(baseVertex + 3);
  m_Indices.push_back(baseVertex + 2);
}

void Chunk::DrawCommand::setIndices()
{
  m_Indices.clear();
  m_Indices.reserve(m_IndexCount);

  for (int i = 0; i < m_Quads.size(); ++i)
    addQuadIndices(4 * i);
}

void Chunk::DrawCommand::setIndices(const GlobalIndex& originIndex, const Vec3& viewPosition)
{
  m_Indices.clear();
  m_Indices.reserve(m_IndexCount);

  Vec3 chunkAnchor = Chunk::AnchorPosition(m_ID, originIndex);

  for (const Voxel& voxel : m_Voxels)
  {
    Vec3 blockCenter = chunkAnchor + Block::Length() * Vec3(voxel.index()) + Vec3(Block::Length()) / 2;
    Vec3 toBlock = blockCenter - viewPosition;

    int quadVertexOffset = 0;
    std::array<int, 6> quadVertexOffsets{ -1, -1, -1, -1, -1, -1 };
    for (Direction face : Directions())
    {
      int faceID = static_cast<int>(face);
      if (voxel.faceEnabled(face))
      {
        quadVertexOffsets[faceID] = quadVertexOffset;
        quadVertexOffset += 4;
      }
    }

    for (int coordID = 0; coordID < 3; ++coordID)
    {
      int faceID = 2 * coordID + (toBlock[coordID] < 0 ? 0 : 1);
      if (quadVertexOffsets[faceID] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[faceID]);
    }

    for (int coordID = 0; coordID < 3; ++coordID)
    {
      int faceID = 2 * coordID + (toBlock[coordID] < 0 ? 1 : 0);
      if (quadVertexOffsets[faceID] >= 0)
        addQuadIndices(voxel.baseVertex() + quadVertexOffsets[faceID]);
    }
  }
}
