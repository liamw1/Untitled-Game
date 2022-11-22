#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"
#include <iostream>

Chunk::Chunk()
  : m_Composition(nullptr), m_NonOpaqueFaces(0), m_QuadCount(0), m_GlobalIndex({})
{
}

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(nullptr), m_NonOpaqueFaces(0), m_QuadCount(0), m_GlobalIndex(chunkIndex)
{
}

Chunk::~Chunk()
{
  clear();
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_Mesh(std::move(other.m_Mesh)),
    m_VertexArray(std::move(other.m_VertexArray)),
    m_GlobalIndex(other.m_GlobalIndex),
    m_NonOpaqueFaces(other.m_NonOpaqueFaces),
    m_QuadCount(other.m_QuadCount)
{
  m_Composition = other.m_Composition;
  other.m_Composition = nullptr;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_Mesh = std::move(other.m_Mesh);
    m_VertexArray = std::move(other.m_VertexArray);
    m_GlobalIndex = other.m_GlobalIndex;
    m_NonOpaqueFaces = other.m_NonOpaqueFaces;
    m_QuadCount = other.m_QuadCount;

    delete[] m_Composition;
    m_Composition = other.m_Composition;
    other.m_Composition = nullptr;
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

Block::Type Chunk::getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const
{
  EN_ASSERT(!isEmpty(), "Chunk is empty!");
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");
  return m_Composition[i * Chunk::Size() * Chunk::Size() + j * Chunk::Size() + k];
}

Block::Type Chunk::getBlockType(const BlockIndex& blockIndex) const
{
  return getBlockType(blockIndex.i, blockIndex.j, blockIndex.k);
}

void Chunk::Initialize(const Shared<Engine::TextureArray>& textureArray)
{
  constexpr uint32_t maxIndices = 6 * 6 * TotalBlocks();

  uint32_t offset = 0;
  uint32_t* indices = new uint32_t[maxIndices];
  for (uint32_t i = 0; i < maxIndices; i += 6)
  {
    // Triangle 1
    indices[i + 0] = offset + 0;
    indices[i + 1] = offset + 1;
    indices[i + 2] = offset + 2;

    // Triangle 2
    indices[i + 3] = offset + 2;
    indices[i + 4] = offset + 3;
    indices[i + 5] = offset + 0;

    offset += 4;
  }
  s_IndexBuffer = Engine::IndexBuffer::Create(indices, maxIndices);

  s_Shader = Engine::Shader::Create("assets/shaders/Chunk.glsl");
  s_TextureArray = textureArray;
  Engine::UniformBuffer::Allocate(s_UniformBinding, sizeof(Uniforms));

  delete[] indices;
}

void Chunk::BindBuffers()
{
  s_Shader->bind();
  Engine::UniformBuffer::Bind(s_UniformBinding);
  s_TextureArray->bind(s_TextureSlot);
}

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");

  if (isEmpty())
  {
    if (blockType == Block::Type::Air)
      return;

    m_Composition = new Block::Type[Chunk::TotalBlocks()];
    for (int i = 0; i < Chunk::TotalBlocks(); ++i)
      m_Composition[i] = Block::Type::Air;
  }

  m_Composition[i * Chunk::Size() * Chunk::Size() + j * Chunk::Size() + k] = blockType;
}

void Chunk::setBlockType(const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

void Chunk::setData(Block::Type* composition)
{
  if (m_Composition)
  {
    EN_WARN("Calling setData on a non-empty chunk!  Deleting previous allocation...");
    delete[] m_Composition;
  }
  m_Composition = composition;
  determineOpacity();
}

void Chunk::setMesh(const uint32_t* meshData, uint16_t quadCount)
{
  m_VertexArray = Engine::VertexArray::Create();
  m_VertexArray->setLayout(s_VertexBufferLayout);
  m_VertexArray->setIndexBuffer(s_IndexBuffer);

  m_QuadCount = quadCount;
  m_VertexArray->setVertexBuffer(meshData, 4 * sizeof(uint32_t) * m_QuadCount);
}

void Chunk::determineOpacity()
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  if (m_Composition == nullptr)
  {
    m_NonOpaqueFaces = 0x3F;
    return;
  }

  m_NonOpaqueFaces = 0;
  for (Block::Face face : Block::FaceIterator())
  {
    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        BlockIndex blockIndex = BlockIndex::CreatePermuted(chunkLimits[IsPositive(face)], i, j, GetCoordID(face));
        if (Block::HasTransparency(getBlockType(blockIndex)))
        {
          m_NonOpaqueFaces |= bit(static_cast<int>(face));
          goto nextFace;
        }
      }
  nextFace:;
  }
}

void Chunk::update()
{
  determineOpacity();
}

void Chunk::draw() const
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(m_QuadCount);

  if (meshIndexCount == 0)
    return; // Nothing to draw

  Uniforms uniforms = { anchorPosition() };
  Engine::UniformBuffer::SetData(s_UniformBinding, &uniforms);
  Engine::RenderCommand::DrawIndexed(m_VertexArray.get(), meshIndexCount);
}

void Chunk::clear()
{
  if (m_Composition)
  {
    delete[] m_Composition;
    m_Composition = nullptr;
  }
}

void Chunk::reset()
{
  clear();

  // Reset data to default values
  m_Mesh.clear();
  m_VertexArray.reset();
  m_GlobalIndex = {};
  m_NonOpaqueFaces = 0;
  m_QuadCount = 0;
}