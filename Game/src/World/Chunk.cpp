#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"

Unique<Engine::Shader> Chunk::s_Shader = nullptr;
Shared<Engine::TextureArray> Chunk::s_TextureArray = nullptr;
Unique<Engine::UniformBuffer> Chunk::s_UniformBuffer = nullptr;
Shared<const Engine::IndexBuffer> Chunk::s_IndexBuffer = nullptr;
const Engine::BufferLayout Chunk::s_VertexBufferLayout = { { ShaderDataType::Uint32, "a_VertexData" } };

Chunk::Chunk()
  : m_Composition(nullptr), m_NonOpaqueFaces(0), m_QuadCount(0), m_GlobalIndex({}) {}

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_Composition(nullptr), m_NonOpaqueFaces(0), m_QuadCount(0), m_GlobalIndex(chunkIndex)
{
  m_VertexArray = Engine::VertexArray::Create();
  m_VertexArray->setLayout(s_VertexBufferLayout);
  m_VertexArray->setIndexBuffer(s_IndexBuffer);
}

Chunk::~Chunk()
{
  clear();
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

bool Chunk::BlockNeighborIsInAnotherChunk(const BlockIndex& blockIndex, Block::Face face)
{
  static constexpr blockIndex_t chunkLimits[2] = { 0, Chunk::Size() - 1 };
  int faceID = static_cast<int>(face);
  int coordID = faceID / 2;

  return blockIndex[coordID] == chunkLimits[faceID % 2];
}

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(0 <= i && i < Chunk::Size() && 0 <= j && j < Chunk::Size() && 0 <= k && k < Chunk::Size(), "Index is out of bounds!");

  if (isEmpty())
  {
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

void Chunk::fill(const HeightMap& heightMap)
{
  if (m_Composition)
  {
    EN_WARN("Calling fill on a non-empty chunk!  Deleting previous allocation...");
    delete[] m_Composition;
  }

  length_t chunkFloor = Chunk::Length() * m_GlobalIndex.k;
  length_t chunkCeiling = chunkFloor + Chunk::Length();

  if (chunkFloor > heightMap.maxHeight)
  {
    m_Composition = nullptr;
    m_NonOpaqueFaces = 0x3F;
    return;
  }
  else
    m_Composition = new Block::Type[Chunk::TotalBlocks()];

  bool isEmpty = true;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      const length_t& terrainHeight = heightMap.surfaceData[i][j].getHeight();
      const Block::Type& surfaceBlockType = heightMap.surfaceData[i][j].getPrimaryBlockType();

      if (terrainHeight < chunkFloor)
      {
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          setBlockType(i, j, k, Block::Type::Air);
      }
      else if (terrainHeight > chunkCeiling + Block::Length())
      {
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          setBlockType(i, j, k, Block::Type::Dirt);
        isEmpty = false;
      }
      else
      {
        int terrainHeightIndex = static_cast<int>((terrainHeight - chunkFloor) / Block::Length());
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
        {
          if (k == terrainHeightIndex)
          {
            setBlockType(i, j, k, surfaceBlockType);
            isEmpty = false;
          }
          else if (k < terrainHeightIndex)
          {
            setBlockType(i, j, k, Block::Type::Dirt);
            isEmpty = false;
          }
          else
            setBlockType(i, j, k, Block::Type::Air);
        }
      }
    }

  if (isEmpty)
  {
    delete[] m_Composition;
    m_Composition = nullptr;
  }

  determineOpacity();
}

void Chunk::fill3D()
{
  static constexpr length_t surfaceHeight = 20 * Block::Length();

  if (m_Composition)
  {
    EN_WARN("Calling fill on a non-empty chunk!  Deleting previous allocation...");
    delete[] m_Composition;
  }

  length_t chunkFloor = Chunk::Length() * m_GlobalIndex.k;

  if (chunkFloor > surfaceHeight)
  {
    m_Composition = nullptr;
    m_NonOpaqueFaces = 0x3F;
    return;
  }
  else
    m_Composition = new Block::Type[Chunk::TotalBlocks()];

  EN_PROFILE_FUNCTION();

  bool isEmpty = true;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
      {
        length_t noiseVal = Noise::FastTerrainNoise3D(Chunk::Length() * static_cast<Vec3>(m_GlobalIndex) + Block::Length() * (Vec3(i, j, k) + Vec3(0.5)));
        if (noiseVal < 0.0)
        {
          setBlockType(i, j, k, Block::Type::Stone);
          isEmpty = false;
        }
        else
          setBlockType(i, j, k, Block::Type::Air);
      }

  if (isEmpty)
  {
    delete[] m_Composition;
    m_Composition = nullptr;
  }

  determineOpacity();
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

  Uniforms uniforms{};
  uniforms.anchorPosition = anchorPosition();

  Engine::Renderer::DrawMesh(m_VertexArray.get(), meshIndexCount, s_UniformBuffer.get(), uniforms);
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
  m_VertexArray.reset();
  m_GlobalIndex = {};
  m_NonOpaqueFaces = 0;
  m_QuadCount = 0;
}



HeightMap::HeightMap(const GlobalIndex& index)
  : chunkI(index.i), chunkJ(index.j), maxHeight(-std::numeric_limits<length_t>::infinity())
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