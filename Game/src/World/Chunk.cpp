#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"
#include <iostream>

Chunk::Chunk() = default;

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(),
    m_Lighting(),
    m_GlobalIndex(chunkIndex),
    m_NonOpaqueFaces(0x3F) {}

Chunk::Chunk(Chunk&& other) noexcept
  : m_Composition(std::move(other.m_Composition)),
    m_Lighting(std::move(other.m_Lighting)),
    m_GlobalIndex(std::move(other.m_GlobalIndex)),
    m_NonOpaqueFaces(other.m_NonOpaqueFaces.load()) {}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_Composition = std::move(other.m_Composition);
    m_Lighting = std::move(other.m_Lighting);
    m_GlobalIndex = std::move(other.m_GlobalIndex);
    m_NonOpaqueFaces.store(other.m_NonOpaqueFaces.load());
  }
  return *this;
}

const GlobalIndex& Chunk::globalIndex() const
{
  return m_GlobalIndex;
}

bool Chunk::empty() const
{
  return !m_Composition;
}

Block::Type Chunk::getBlockType(const BlockIndex& blockIndex) const
{
  return getBlockType(blockIndex.i, blockIndex.j, blockIndex.k);
}

Block::Type Chunk::getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const
{
  return m_Composition[i][j][k];
}

Block::Light Chunk::getBlockLight(const BlockIndex& blockIndex) const
{
  return getBlockLight(blockIndex.i, blockIndex.j, blockIndex.k);
}

Block::Light Chunk::getBlockLight(blockIndex_t i, blockIndex_t j, blockIndex_t k) const
{
  return m_Lighting[i][j][k];
}

bool Chunk::isFaceOpaque(Direction face) const
{
  uint16_t nonOpaqueFaces = m_NonOpaqueFaces.load();
  return !(nonOpaqueFaces & bit(static_cast<int>(face)));
}

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  if (empty())
  {
    if (blockType == Block::Type::Air)
      return;

    m_Composition = AllocateArray3D<Block::Type, c_ChunkSize>(Block::Type::Air);
    m_Lighting = AllocateArray3D<Block::Light, c_ChunkSize>(15);
  }
  m_Composition[i][j][k] = blockType;
}

void Chunk::setBlockType(const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

void Chunk::setBlockLight(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Light blockLight)
{
  if (empty())
  {
    if (blockLight.sunlight() == Block::Light::MaxValue())
      return;

    m_Composition = AllocateArray3D<Block::Type, c_ChunkSize>(Block::Type::Air);
    m_Lighting = AllocateArray3D<Block::Light, c_ChunkSize>(15);
  }
  m_Lighting[i][j][k] = blockLight;
}

void Chunk::setBlockLight(const BlockIndex& blockIndex, Block::Light blockLight)
{
  setBlockLight(blockIndex.i, blockIndex.j, blockIndex.k, blockLight);
}

void Chunk::setComposition(Array3D<Block::Type, c_ChunkSize>&& composition)
{
  if (m_Composition)
    EN_WARN("Calling setComposition on a non-empty chunk!  Deleting previous allocation...");
  m_Composition = std::move(composition);
  determineOpacity();
}

void Chunk::setLighting(Array3D<Block::Light, c_ChunkSize>&& lighting)
{
  if (m_Lighting)
    EN_WARN("Calling setLighting on an already lit chunk!  Deleting previous allocation...");
  m_Lighting = std::move(lighting);
  determineOpacity();
}

void Chunk::determineOpacity()
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  if (empty())
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
        if (Block::HasTransparency(getBlockType(blockIndex)))
        {
          nonOpaqueFaces |= bit(static_cast<int>(face));
          goto nextFace;
        }
      }
  nextFace:;
  }
  m_NonOpaqueFaces.store(nonOpaqueFaces);
}

void Chunk::update(bool hasMesh)
{
  if (!empty() && !hasMesh && getBlockType(0, 0, 0) == Block::Type::Air)
  {
    m_Composition.reset();
    m_Lighting.reset();
  }

  determineOpacity();
}

void Chunk::reset()
{
  // Reset data to default values
  m_Composition.reset();
  m_Lighting.reset();
  m_GlobalIndex = {};
  m_NonOpaqueFaces.store(0x3F);
}

Vec3 Chunk::Center(const Vec3& anchorPosition)
{
  return anchorPosition + Chunk::Length() / 2;
}

Vec3 Chunk::AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex)
{
  return Chunk::Length() * static_cast<Vec3>(chunkIndex - originIndex);
}

_Acquires_lock_(return) std::unique_lock<std::mutex> Chunk::acquireLock() const
{
  return std::unique_lock(m_Mutex);
}

_Acquires_lock_(return) std::lock_guard<std::mutex> Chunk::acquireLockGuard() const
{
  return std::lock_guard(m_Mutex);
}



Chunk::Vertex::Vertex()
  : m_Data(0) {}
Chunk::Vertex::Vertex(const BlockIndex& vertexPlacement, int quadIndex, int ambientOcclusion, Block::Texture texture)
{
  m_Data  = vertexPlacement.i + (vertexPlacement.j << 6) + (vertexPlacement.k << 12);
  m_Data |= quadIndex << 18;
  m_Data |= ambientOcclusion << 20;
  m_Data |= static_cast<blockTexID>(texture) << 22;
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



Chunk::Quad::Quad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, std::array<int, 4> ambientOcclusion)
{
  static constexpr std::array<int, 4> standardOrder = { 0, 1, 2, 3 };
  static constexpr std::array<int, 4> reversedOrder = { 1, 3, 0, 2 };

  int ambientOcclusionDifferenceAlongStandardSeam = std::abs(ambientOcclusion[1] - ambientOcclusion[2]);
  int ambientOcclusionDifferenceAlongReversedSeam = std::abs(ambientOcclusion[0] - ambientOcclusion[3]);
  const std::array<int, 4>& quadOrder = ambientOcclusionDifferenceAlongStandardSeam > ambientOcclusionDifferenceAlongReversedSeam ? reversedOrder : standardOrder;
  for (int i = 0; i < 4; ++i)
  {
    int quadIndex = quadOrder[i];
    BlockIndex vertexPlacement = blockIndex + Vertex::GetOffset(face, quadIndex);
    m_Vertices[i] = Vertex(vertexPlacement, quadIndex, ambientOcclusion[quadIndex], texture);
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

void Chunk::DrawCommand::addQuad(const BlockIndex& blockIndex, Direction face, Block::Texture texture, std::array<int, 4> ambientOcclusion)
{
  m_Quads.emplace_back(blockIndex, face, texture, ambientOcclusion);
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

  EN_PROFILE_FUNCTION();

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
