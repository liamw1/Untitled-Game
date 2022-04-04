#include "GMpch.h"
#include "NewChunk.h"
#include "Player/Player.h"

Unique<Engine::Shader> Chunk::s_Shader = nullptr;
Shared<Engine::TextureArray> Chunk::s_TextureArray = nullptr;
Unique<Engine::UniformBuffer> Chunk::s_UniformBuffer = nullptr;
Shared<const Engine::IndexBuffer> Chunk::s_IndexBuffer = nullptr;
const Engine::BufferLayout Chunk::s_VertexBufferLayout = { { ShaderDataType::Uint32, "a_VertexData" } };

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(nullptr), m_NonOpaqueFaces(0), m_QuadCount(0), m_GlobalIndex(chunkIndex)
{
  m_VertexArray = Engine::VertexArray::Create();
  m_VertexArray->setLayout(s_VertexBufferLayout);
  m_VertexArray->setIndexBuffer(s_IndexBuffer);
}

Chunk::~Chunk()
{
  if (m_Composition)
  {
    delete[] m_Composition;
    m_Composition = nullptr;
  }
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_GlobalIndex(other.m_GlobalIndex),
    m_NonOpaqueFaces(other.m_NonOpaqueFaces),
    m_QuadCount(other.m_QuadCount),
    m_VertexArray(std::move(other.m_VertexArray))
{
  m_Composition = other.m_Composition;
  other.m_Composition = nullptr;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_GlobalIndex = other.m_GlobalIndex;
    m_NonOpaqueFaces = other.m_NonOpaqueFaces;
    m_QuadCount = other.m_QuadCount;
    m_VertexArray = std::move(other.m_VertexArray);

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

void Chunk::draw() const
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(m_QuadCount);

  if (meshIndexCount == 0)
    return; // Nothing to draw

  Uniforms uniforms{};
  uniforms.anchorPosition = anchorPosition();

  Engine::Renderer::DrawMesh(m_VertexArray.get(), meshIndexCount, s_UniformBuffer.get(), uniforms);
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
  s_UniformBuffer = Engine::UniformBuffer::Create(sizeof(Uniforms), 1);

  delete[] indices;
}

void Chunk::BindBuffers()
{
  s_Shader->bind();
  s_TextureArray->bind(s_TextureSlot);
}

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(m_Composition, "Chunk data has not yet been allocated!");
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");

  if (isEmpty())
  {
    m_Composition = new Block::Type[Chunk::TotalBlocks()];
    for (blockIndex_t i = 0; i < Chunk::TotalBlocks(); ++i)
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
  m_QuadCount = quadCount;
  m_VertexArray->setVertexBuffer(meshData, 4 * sizeof(uint32_t) * m_QuadCount);
}

void Chunk::determineOpacity()
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };

  m_NonOpaqueFaces = 0;
  for (Block::Face face : Block::FaceIterator())
  {
    const int faceID = static_cast<int>(face);

    // Relabel coordinates
    const int u = faceID / 2;
    const int v = (u + 1) % 3;
    const int w = (u + 2) % 3;

    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      {
        BlockIndex blockIndex{};
        blockIndex[u] = chunkLimits[faceID % 2];
        blockIndex[v] = i;
        blockIndex[w] = j;

        if (Block::HasTransparency(getBlockType(blockIndex)))
        {
          m_NonOpaqueFaces |= bit(faceID);
          goto nextFace;
        }
      }
  nextFace:;
  }
}

HeightMap::HeightMap(globalIndex_t chunkI, globalIndex_t chunkJ)
  : chunkI(chunkI), chunkJ(chunkJ), maxHeight(-std::numeric_limits<length_t>::infinity())
{
  EN_PROFILE_FUNCTION();

  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      Vec2 blockXY = Chunk::Length() * Vec2(chunkI, chunkJ) + Block::Length() * (Vec2(i, j) + Vec2(0.5));
      surfaceData[i][j] = Noise::FastTerrainNoise2D(blockXY);

      if (surfaceData[i][j].getHeight() > maxHeight)
        maxHeight = surfaceData[i][j].getHeight();
    }
}