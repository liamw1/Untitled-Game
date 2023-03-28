#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"
#include <iostream>

Chunk::Chunk()
  : m_Composition(nullptr),
    m_NonOpaqueFaces(0),
    m_QuadCount(0),
    m_GlobalIndex({})
{
}

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(nullptr),
    m_NonOpaqueFaces(0),
    m_QuadCount(0),
    m_GlobalIndex(chunkIndex)
{
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_Mesh(std::move(other.m_Mesh)),
    m_VertexArray(std::move(other.m_VertexArray)),
    m_Composition(std::move(other.m_Composition)),
    m_GlobalIndex(other.m_GlobalIndex),
    m_NonOpaqueFaces(other.m_NonOpaqueFaces.load()),
    m_QuadCount(other.m_QuadCount)
{
}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_Mesh = std::move(other.m_Mesh);
    m_VertexArray = std::move(other.m_VertexArray);
    m_Composition = std::move(other.m_Composition);
    m_GlobalIndex = other.m_GlobalIndex;
    m_NonOpaqueFaces.store(other.m_NonOpaqueFaces.load());
    m_QuadCount = other.m_QuadCount;
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
  EN_ASSERT(!empty(), "Chunk is empty!");
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");
  return m_Composition[i * Chunk::Size() * Chunk::Size() + j * Chunk::Size() + k];
}

Block::Type Chunk::getBlockType(const BlockIndex& blockIndex) const
{
  return getBlockType(blockIndex.i, blockIndex.j, blockIndex.k);
}

bool Chunk::isFaceOpaque(Block::Face face) const
{
  uint16_t nonOpaqueFaces = m_NonOpaqueFaces.load();
  return !(nonOpaqueFaces & bit(static_cast<int>(face)));
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
  Engine::UniformBuffer::Allocate(c_UniformBinding, sizeof(Uniforms));

  delete[] indices;
}

void Chunk::BindBuffers()
{
  s_Shader->bind();
  Engine::UniformBuffer::Bind(c_UniformBinding);
  s_TextureArray->bind(c_TextureSlot);
}

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");

  if (empty())
  {
    if (blockType == Block::Type::Air)
      return;

    // Elements will be default initialized to 0 (Air)
    m_Composition = std::make_unique<Block::Type[]>(Chunk::TotalBlocks());
  }

  m_Composition[i * Chunk::Size() * Chunk::Size() + j * Chunk::Size() + k] = blockType;
}

void Chunk::setBlockType(const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

void Chunk::setData(std::unique_ptr<Block::Type[]> composition)
{
  if (m_Composition)
    EN_WARN("Calling setData on a non-empty chunk!  Deleting previous allocation...");
  m_Composition = std::move(composition);
  determineOpacity();
}

void Chunk::uploadMesh()
{
  if (m_Mesh.empty())
    return;

  if (!m_VertexArray)
  {
    m_VertexArray = Engine::VertexArray::Create();
    m_VertexArray->setLayout(s_VertexBufferLayout);
    m_VertexArray->setIndexBuffer(s_IndexBuffer);
  }

  m_QuadCount = static_cast<uint16_t>(m_Mesh.size() / 4);
  m_VertexArray->setVertexBuffer(m_Mesh.data(), 4 * sizeof(uint32_t) * m_QuadCount);
  m_Mesh.clear();
}

void Chunk::determineOpacity()
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  if (m_Composition == nullptr)
  {
    m_NonOpaqueFaces.store(0x3F);
    return;
  }

  uint16_t nonOpaqueFaces = 0;
  for (Block::Face face : Block::FaceIterator())
  {
    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        BlockIndex blockIndex = BlockIndex::CreatePermuted(chunkLimits[IsPositive(face)], i, j, GetCoordID(face));
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

void Chunk::internalUpdate(const std::vector<uint32_t>& mesh)
{
  EN_ASSERT(mesh.size() / 4 < std::numeric_limits<uint16_t>::max(), "Mesh has more than the maximum allowable number of quads!");

  m_Mesh = mesh;
  if (!empty() && m_Mesh.empty() && getBlockType(0, 0, 0) == Block::Type::Air)
    m_Composition.reset();

  determineOpacity();
}

void Chunk::draw() const
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(m_QuadCount);

  if (meshIndexCount == 0)
    return; // Nothing to draw

  Uniforms uniforms = { anchorPosition() };
  Engine::UniformBuffer::SetData(c_UniformBinding, &uniforms);
  Engine::RenderCommand::DrawIndexed(m_VertexArray.get(), meshIndexCount);
}

void Chunk::reset()
{
  // Reset data to default values
  m_Mesh.clear();
  m_VertexArray.reset();
  m_Composition.reset();
  m_GlobalIndex = {};
  m_NonOpaqueFaces.store(0);
  m_QuadCount = 0;
}