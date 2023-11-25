#include "GMpch.h"
#include "Chunk.h"
#include "Indexing/Operations.h"

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(Bounds(), block::ID::Air),
    m_Lighting(Bounds(), block::Light::MaxValue()),
    m_NonOpaqueFaces(0x3F),
    m_GlobalIndex(chunkIndex) {}

const GlobalIndex& Chunk::globalIndex() const
{
  return m_GlobalIndex;
}

ProtectedBlockArrayBox<block::Type>& Chunk::composition()
{
  ENG_MUTABLE_VERSION(composition);
}

const ProtectedBlockArrayBox<block::Type>& Chunk::composition() const
{
  return m_Composition;
}

ProtectedBlockArrayBox<block::Light>& Chunk::lighting()
{
  ENG_MUTABLE_VERSION(lighting);
}

const ProtectedBlockArrayBox<block::Light>& Chunk::lighting() const
{
  return m_Lighting;
}

eng::math::Vec3 Chunk::center(const GlobalIndex& originIndex) const
{
  return indexCenter(globalIndex(), originIndex);
}

eng::math::Vec3 Chunk::anchorPosition(const GlobalIndex& originIndex) const
{
  return indexPosition(globalIndex(), originIndex);
}

bool Chunk::isFaceOpaque(eng::math::Direction face) const
{
  u16 nonOpaqueFaces = m_NonOpaqueFaces.load();
  return !(nonOpaqueFaces & eng::bit(eng::toUnderlying(face)));
}

void Chunk::setComposition(BlockArrayBox<block::Type>&& composition)
{
  m_Composition.setData(std::move(composition));
  determineOpacity();
}

void Chunk::setLighting(BlockArrayBox<block::Light>&& lighting)
{
  m_Lighting.setData(std::move(lighting));
}

void Chunk::determineOpacity()
{
  m_Composition.readOperation([this](const BlockArrayBox<block::Type>& arrayBox)
  {
    if (!arrayBox)
    {
      m_NonOpaqueFaces.store(0x3F);
      return;
    }

    u16 nonOpaqueFaces = 0;
    for (eng::math::Direction direction : eng::math::Directions())
    {
      BlockBox face = Bounds().face(direction);
      if (arrayBox.anyOf(face, [](block::Type blockType) { return blockType.hasTransparency(); }))
        nonOpaqueFaces |= eng::bit(eng::toUnderlying(direction));
    }
    m_NonOpaqueFaces.store(nonOpaqueFaces);
  });
}

void Chunk::update()
{
  m_Composition.clearIfFilledWithDefault();
  m_Lighting.clearIfFilledWithDefault();

  determineOpacity();
}