#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"

Chunk::Chunk() = default;

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(),
    m_Lighting(),
    m_NonOpaqueFaces(0x3F) {}

Chunk::ArrayBox<Block::Type>& Chunk::composition()
{
  return const_cast<ArrayBox<Block::Type>&>(static_cast<const Chunk*>(this)->composition());
}

const Chunk::ArrayBox<Block::Type>& Chunk::composition() const
{
  return m_Composition;
}

Chunk::ArrayBox<Block::Light>& Chunk::lighting()
{
  return const_cast<ArrayBox<Block::Light>&>(static_cast<const Chunk*>(this)->lighting());
}

const Chunk::ArrayBox<Block::Light>& Chunk::lighting() const
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

    m_Composition = ArrayBox<Block::Type>(Block::Type::Air);
  }
  m_Composition(blockIndex) = blockType;
}

void Chunk::setBlockLight(const BlockIndex& blockIndex, Block::Light blockLight)
{
  if (!m_Lighting)
  {
    if (blockLight.sunlight() == Block::Light::MaxValue())
      return;

    m_Lighting = ArrayBox<Block::Light>(Block::Light::MaxValue());
  }
  m_Lighting(blockIndex) = blockLight;
}

void Chunk::setComposition(ArrayBox<Block::Type>&& composition)
{
  if (m_Composition)
    EN_WARN("Calling setComposition on a non-empty chunk!  Deleting previous allocation...");
  m_Composition = std::move(composition);
  determineOpacity();
}

void Chunk::setLighting(ArrayBox<Block::Light>&& lighting)
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
  for (Direction direction : Directions())
  {
    BlockBox face = Bounds().face(direction);
    if (m_Composition.anyOf(face, [](Block::Type blockType) { return Block::HasTransparency(blockType); }))
      nonOpaqueFaces |= Engine::Bit(static_cast<int>(direction));
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

Vec3 Chunk::Center(const Vec3& anchorPosition)
{
  return anchorPosition + Chunk::Length() / 2;
}

Vec3 Chunk::AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex)
{
  return Chunk::Length() * static_cast<Vec3>(chunkIndex - originIndex);
}