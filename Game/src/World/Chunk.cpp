#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(Block::Type::Air),
    m_Lighting(Block::Light::MaxValue()),
    m_NonOpaqueFaces(0x3F) {}

Chunk::ProtectedArrayBox<Block::Type>& Chunk::composition()
{
  return m_Composition;
}

const Chunk::ProtectedArrayBox<Block::Type>& Chunk::composition() const
{
  return m_Composition;
}

Chunk::ProtectedArrayBox<Block::Light>& Chunk::lighting()
{
  return m_Lighting;
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