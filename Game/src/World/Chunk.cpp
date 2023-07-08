#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"
#include <iostream>

Chunk::Chunk()
  : m_Composition(),
    m_GlobalIndex(),
    m_NonOpaqueFaces(0)
{
}

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(),
    m_GlobalIndex(chunkIndex),
    m_NonOpaqueFaces(0)
{
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_Composition(std::move(other.m_Composition)),
    m_GlobalIndex(other.m_GlobalIndex),
    m_NonOpaqueFaces(other.m_NonOpaqueFaces.load())
{
}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_Composition = std::move(other.m_Composition);
    m_GlobalIndex = other.m_GlobalIndex;
    m_NonOpaqueFaces.store(other.m_NonOpaqueFaces.load());
  }
  return *this;
}

LocalIndex Chunk::getLocalIndex() const
{
  EN_ASSERT(abs(m_GlobalIndex.i - Player::OriginIndex().i) < std::numeric_limits<localIndex_t>::max() &&
            abs(m_GlobalIndex.j - Player::OriginIndex().j) < std::numeric_limits<localIndex_t>::max() &&
            abs(m_GlobalIndex.k - Player::OriginIndex().k) < std::numeric_limits<localIndex_t>::max(), "Difference between global indices is too large, will cause overflow!");

  return { static_cast<localIndex_t>(m_GlobalIndex.i - Player::OriginIndex().i),
           static_cast<localIndex_t>(m_GlobalIndex.j - Player::OriginIndex().j),
           static_cast<localIndex_t>(m_GlobalIndex.k - Player::OriginIndex().k) };
}

Vec3 Chunk::anchorPosition() const
{
  return Chunk::AnchorPosition(m_GlobalIndex, Player::OriginIndex());
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



blockIndex_t Chunk::Voxel::x() const
{
  return (m_VoxelData & 0x001F0000u) >> 16u;
}

blockIndex_t Chunk::Voxel::y() const
{
  return (m_VoxelData & 0x03E00000u) >> 21u;
}

blockIndex_t Chunk::Voxel::z() const
{
  return (m_VoxelData & 0x7C000000u) >> 26u;
}

BlockIndex Chunk::Voxel::index() const
{
  return { x(), y(), z() };
}

void Chunk::DrawCommand::sortVoxels(const GlobalIndex& originIndex, const Vec3& playerPosition)
{
  BlockIndex playerBlock = BlockIndex::ToIndex(playerPosition / Block::Length());
  BlockIndex originBlock = playerBlock;
  for (int i = 0; i < 3; ++i)
  {
    if (originIndex[i] > id()[i])
      originBlock[i] = Chunk::Size() - 1;
    else if (originIndex[i] < id()[i])
      originBlock[i] = 0;
  }
  if (originBlock == m_SortState)
    return;

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