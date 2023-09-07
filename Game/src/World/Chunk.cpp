#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(Block::Type::Air),
    m_Lighting(Block::Light::MaxValue()),
    m_NonOpaqueFaces(0x3F) {}

Chunk::ProtectedArrayBox<Block::Type>& Chunk::composition()
{
  return const_cast<ProtectedArrayBox<Block::Type>&>(static_cast<const Chunk*>(this)->composition());
}

const Chunk::ProtectedArrayBox<Block::Type>& Chunk::composition() const
{
  return m_Composition;
}

Chunk::ProtectedArrayBox<Block::Light>& Chunk::lighting()
{
  return const_cast<ProtectedArrayBox<Block::Light>&>(static_cast<const Chunk*>(this)->lighting());
}

const Chunk::ProtectedArrayBox<Block::Light>& Chunk::lighting() const
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
  m_Composition.set(blockIndex, blockType);
}

void Chunk::setBlockLight(const BlockIndex& blockIndex, Block::Light blockLight)
{
  m_Lighting.set(blockIndex, blockLight);
}

void Chunk::setComposition(ArrayBox<Block::Type>&& composition)
{
  m_Composition.setData(std::move(composition));
  determineOpacity();
}

void Chunk::setLighting(ArrayBox<Block::Light>&& lighting)
{
  m_Lighting.setData(std::move(lighting));
}

void Chunk::determineOpacity()
{
  m_Composition.readOperation([this](const ArrayBox<Block::Type>& arrayBox)
    {
      if (!arrayBox)
      {
        m_NonOpaqueFaces.store(0x3F);
        return;
      }

      uint16_t nonOpaqueFaces = 0;
      for (Direction direction : Directions())
      {
        BlockBox face = Bounds().face(direction);
        if (arrayBox.anyOf(face, [](Block::Type blockType) { return Block::HasTransparency(blockType); }))
          nonOpaqueFaces |= Engine::Bit(static_cast<int>(direction));
      }
      m_NonOpaqueFaces.store(nonOpaqueFaces);
    });
}

void Chunk::update()
{
  m_Composition.resetIfFilledWithDefault();
  m_Lighting.resetIfFilledWithDefault();

  determineOpacity();
}

Vec3 Chunk::Center(const Vec3& anchorPosition)
{
  return anchorPosition + Chunk::Length() / 2;
}

Vec3 Chunk::AnchorPosition(const GlobalIndex& chunkIndex, const GlobalIndex& originIndex)
{
  return Chunk::Length() * static_cast<Vec3>(chunkIndex - originIndex);
}