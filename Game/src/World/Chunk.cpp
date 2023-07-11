#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"
#include <iostream>

Chunk::Chunk() = default;

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(),
    m_GlobalIndex(chunkIndex),
    m_NonOpaqueFaces(0) {}

Chunk::Chunk(Chunk&& other) noexcept
  : m_Composition(std::move(other.m_Composition)),
    m_GlobalIndex(std::move(other.m_GlobalIndex)),
    m_NonOpaqueFaces(other.m_NonOpaqueFaces.load()) {}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_Composition = std::move(other.m_Composition);
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

Block::Type Chunk::getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const
{
  return m_Composition[i][j][k];
}

Block::Type Chunk::getBlockType(const BlockIndex& blockIndex) const
{
  return getBlockType(blockIndex.i, blockIndex.j, blockIndex.k);
}

bool Chunk::isFaceOpaque(Direction face) const
{
  uint16_t nonOpaqueFaces = m_NonOpaqueFaces.load();
  return !(nonOpaqueFaces & bit(static_cast<int>(face)));
}

Vec3 Chunk::Center(const Vec3& anchorPosition)
{
  return anchorPosition + Chunk::Length() / 2;
}

Vec3 Chunk::AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex)
{
  return Chunk::Length() * static_cast<Vec3>(chunkIndex - originIndex);
}

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  if (empty())
  {
    if (blockType == Block::Type::Air)
      return;

    // Elements will be default initialized to Air
    m_Composition = AllocateArray3D<Block::Type, c_ChunkSize>(Block::Type::Air);
  }
  m_Composition[i][j][k] = blockType;
}

void Chunk::setBlockType(const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

void Chunk::setData(Array3D<Block::Type, c_ChunkSize> composition)
{
  if (m_Composition)
    EN_WARN("Calling setData on a non-empty chunk!  Deleting previous allocation...");
  m_Composition = std::move(composition);
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
    m_Composition.reset();

  determineOpacity();
}

void Chunk::reset()
{
  // Reset data to default values
  m_Composition.reset();
  m_GlobalIndex = {};
  m_NonOpaqueFaces.store(0);
}

_Acquires_lock_(return) std::lock_guard<std::mutex> Chunk::acquireLock() const
{
  return std::lock_guard(m_Mutex);
};



Chunk::Voxel::Voxel() = default;
Chunk::Voxel::Voxel(uint32_t voxelData, uint32_t quadData1, uint32_t quadData2)
  : m_VoxelData(voxelData), m_QuadData1(quadData1), m_QuadData2(quadData2) {}

blockIndex_t Chunk::Voxel::i() const { return (m_VoxelData & 0x001F0000u) >> 16u; }
blockIndex_t Chunk::Voxel::j() const { return (m_VoxelData & 0x03E00000u) >> 21u; }
blockIndex_t Chunk::Voxel::k() const { return (m_VoxelData & 0x7C000000u) >> 26u; }
BlockIndex Chunk::Voxel::index() const { return { i(), j(), k() }; }



Chunk::DrawCommand::DrawCommand(const GlobalIndex& chunkIndex)
  : Engine::MultiDrawCommand<GlobalIndex, DrawCommand>(chunkIndex, 0),
    m_Mesh() {}
Chunk::DrawCommand::DrawCommand(const GlobalIndex& chunkIndex, std::vector<Voxel>&& mesh)
  : Engine::MultiDrawCommand<GlobalIndex, DrawCommand>(chunkIndex, static_cast<int>(mesh.size())),
    m_Mesh(std::move(mesh)) {}

Chunk::DrawCommand::DrawCommand(DrawCommand&& other) noexcept = default;
Chunk::DrawCommand& Chunk::DrawCommand::operator=(DrawCommand&& other) noexcept = default;

bool Chunk::DrawCommand::operator==(const DrawCommand& other) const
{
  return m_ID == other.m_ID;
}

const void* Chunk::DrawCommand::vertexData() const
{
  return m_Mesh.data();
}

void Chunk::DrawCommand::sortVoxels(const GlobalIndex& originIndex, const Vec3& position)
{
  // Find block index that is closest to the specified position
  BlockIndex playerBlock = BlockIndex::ToIndex(position / Block::Length());
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
    return;

  EN_PROFILE_FUNCTION();

  // Sort based on L1 distance to originBlock
  std::sort(m_Mesh.begin(), m_Mesh.end(), [&originBlock](const Voxel& voxelA, const Voxel& voxelB)
    {
      BlockIndex diffA = voxelA.index() - originBlock;
      int distA = std::abs(diffA.i) + std::abs(diffA.j) + std::abs(diffA.k);

      BlockIndex diffB = voxelB.index() - originBlock;
      int distB = std::abs(diffB.i) + std::abs(diffB.j) + std::abs(diffB.k);

      return distA > distB;
    });
  m_SortState = originBlock;
}