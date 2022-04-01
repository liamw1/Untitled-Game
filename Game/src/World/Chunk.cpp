#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"

Unique<Engine::Shader> Chunk::s_Shader = nullptr;
Shared<Engine::TextureArray> Chunk::s_TextureArray = nullptr;
Unique<Engine::UniformBuffer> Chunk::s_UniformBuffer = nullptr;
Shared<const Engine::IndexBuffer> Chunk::s_IndexBuffer = nullptr;
const Engine::BufferLayout Chunk::s_VertexBufferLayout = { { ShaderDataType::Uint32, "a_VertexData" } };

Chunk::Chunk()
  : m_GlobalIndex(0, 0, 0) {}

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_GlobalIndex(chunkIndex)
{
  m_VertexArray = Engine::VertexArray::Create();
  m_VertexArray->setLayout(s_VertexBufferLayout);
  m_VertexArray->setIndexBuffer(s_IndexBuffer);
}

Chunk::~Chunk()
{
  excise();
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_GlobalIndex(std::move(other.m_GlobalIndex)),
    m_ChunkComposition(std::move(other.m_ChunkComposition)),
    m_NonOpaqueFaces(std::move(other.m_NonOpaqueFaces)),
    m_MeshState(std::move(other.m_MeshState)),
    m_QuadCount(std::move(other.m_QuadCount)),
    m_VertexArray(std::move(other.m_VertexArray))
{
  // Transfer neighbor pointers
  m_Neighbors = other.m_Neighbors;
  for (Block::Face face : Block::FaceIterator())
    other.m_Neighbors[static_cast<int>(face)] = nullptr;

  // Since the address of 'other' has moved, we must update its neighbors about the change
  sendAddressUpdate();
}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_GlobalIndex = std::move(other.m_GlobalIndex);
    m_ChunkComposition = std::move(other.m_ChunkComposition);
    m_NonOpaqueFaces = std::move(other.m_NonOpaqueFaces);
    m_MeshState = std::move(other.m_MeshState);
    m_QuadCount = std::move(other.m_QuadCount);
    m_VertexArray = std::move(other.m_VertexArray);

    // Transfer neighbor pointers
    m_Neighbors = other.m_Neighbors;
    for (Block::Face face : Block::FaceIterator())
      other.m_Neighbors[static_cast<int>(face)] = nullptr;

    // Since the address of 'other' has moved, we must update its neighbors about the change
    sendAddressUpdate();
  }
  return *this;
}

void Chunk::load(const HeightMap& heightMap)
{
  const length_t chunkHeight = s_ChunkLength * getGlobalIndex().k;

  if (heightMap.maxHeight < chunkHeight)
  {
    markAsEmpty();
    return;
  }

  m_ChunkComposition = CreateUnique<Block::Type[]>(s_ChunkTotalBlocks);

  bool isEmpty = true;
  for (blockIndex_t i = 0; i < s_ChunkSize; ++i)
    for (blockIndex_t j = 0; j < s_ChunkSize; ++j)
    {
      const length_t& terrainHeight = heightMap.surfaceData[i][j].getHeight();
      const Block::Type& surfaceBlockType = heightMap.surfaceData[i][j].getPrimaryBlockType();

      if (terrainHeight < chunkHeight)
      {
        for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, Block::Type::Air);
      }
      else if (terrainHeight > chunkHeight + s_ChunkLength + Block::Length())
      {
        for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, Block::Type::Dirt);
        isEmpty = false;
      }
      else
      {
        const int terrainHeightIndex = static_cast<int>((terrainHeight - chunkHeight) / Block::Length());
        for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
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
    markAsEmpty();
  else
    determineOpacity();
}

void Chunk::update()
{
  determineOpacity();

  if (isEmpty())
    return;

  if (m_MeshState == MeshState::NotGenerated || m_MeshState == MeshState::NeedsUpdate)
    generateMesh();

  if (m_ChunkComposition[0] == Block::Type::Air && m_QuadCount == 0)
    markAsEmpty();
}

void Chunk::draw()
{
  uint32_t meshIndexCount = 6 * static_cast<uint32_t>(m_QuadCount);

  if (meshIndexCount == 0)
    return; // Nothing to draw

  Uniforms uniforms{};
  uniforms.anchorPosition = anchorPosition();

  Engine::Renderer::DrawMesh(m_VertexArray.get(), meshIndexCount, s_UniformBuffer.get(), uniforms);
}

void Chunk::reset()
{
  // Reset data to default values
  m_ChunkComposition.reset();
  m_NonOpaqueFaces = 0;
  m_MeshState = MeshState::NotGenerated;
  m_QuadCount = 0;
  m_VertexArray.reset();

  // Ensure no further communication between other chunks
  excise();
  m_Neighbors = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
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
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);
  EN_ASSERT(0 <= i && i < chunkSize && 0 <= j && j < chunkSize && 0 <= k && k < chunkSize, "Index is out of bounds!");
  return isEmpty() ? Block::Type::Air : m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

Block::Type Chunk::getBlockType(const BlockIndex& blockIndex) const
{
  return getBlockType(blockIndex.i, blockIndex.j, blockIndex.k);
}

Block::Type Chunk::getBlockType(const Vec3& position) const
{
  return getBlockType(blockIndexFromPos(position));
}

void Chunk::removeBlock(const BlockIndex& blockIndex)
{
  setBlockType(blockIndex, Block::Type::Air);

  m_MeshState = MeshState::NeedsUpdate;
  for (Block::Face face : Block::FaceIterator())
  {
    const int faceID = static_cast<int>(face);

    if (isBlockNeighborInAnotherChunk(blockIndex, face) && m_Neighbors[faceID] != nullptr)
      m_Neighbors[faceID]->m_MeshState = MeshState::NeedsUpdate;
  }
}

void Chunk::placeBlock(const BlockIndex& blockIndex, Block::Face face, Block::Type blockType)
{
  EN_ASSERT(!isEmpty(), "Place block cannot be called on an empty chunk!");

  // If trying to place an air block or trying to place a block in a space occupied by another block, do nothing
  if (blockType == Block::Type::Air || !isBlockNeighborAir(blockIndex, face))
    return;

  const int faceID = static_cast<int>(face);
  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    if (m_Neighbors[faceID] == nullptr)
      return;
    else if (m_Neighbors[faceID]->isEmpty())
      m_Neighbors[faceID]->m_ChunkComposition = CreateUnique<Block::Type[]>(s_ChunkTotalBlocks);

    m_Neighbors[faceID]->setBlockType(blockIndex - static_cast<blockIndex_t>(s_ChunkSize - 1) * BlockIndex::OutwardNormal(face), blockType);
    m_Neighbors[faceID]->m_MeshState = MeshState::NeedsUpdate;
  }
  else
    setBlockType(blockIndex + BlockIndex::OutwardNormal(face), blockType);
  m_MeshState = MeshState::NeedsUpdate;
}

void Chunk::setNeighbor(Block::Face face, Chunk* chunk)
{
  m_Neighbors[static_cast<int>(face)] = chunk;
  m_MeshState = MeshState::NeedsUpdate;
}

bool Chunk::isBlockNeighborInAnotherChunk(const BlockIndex& blockIndex, Block::Face face)
{
  static constexpr blockIndex_t chunkLimits[2] = { s_ChunkSize - 1, 0 };
  const int faceID = static_cast<int>(face);
  const int coordID = faceID / 2;

  return blockIndex[coordID] == chunkLimits[faceID % 2];
}

bool Chunk::isBlockNeighborTransparent(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Face face)
{
  return isBlockNeighborTransparent(BlockIndex(i, j, k), face);
}

bool Chunk::isBlockNeighborTransparent(const BlockIndex& blockIndex, Block::Face face)
{
  const int faceID = static_cast<int>(face);
  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    if (m_Neighbors[faceID] == nullptr)
      return false;
    else if (m_Neighbors[faceID]->isEmpty())
      return true;

    return Block::HasTransparency(m_Neighbors[faceID]->getBlockType(blockIndex - static_cast<blockIndex_t>(s_ChunkSize - 1) * BlockIndex::OutwardNormal(face)));
  }
  else
    return Block::HasTransparency(getBlockType(blockIndex + BlockIndex::OutwardNormal(face)));
}

bool Chunk::isBlockNeighborAir(const BlockIndex& blockIndex, Block::Face face)
{
  const int faceID = static_cast<int>(face);
  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    if (m_Neighbors[faceID] == nullptr)
      return false;
    else if (m_Neighbors[faceID]->isEmpty())
      return true;

    return m_Neighbors[faceID]->getBlockType(blockIndex - static_cast<blockIndex_t>(s_ChunkSize - 1) * BlockIndex::OutwardNormal(face)) == Block::Type::Air;
  }
  else
    return getBlockType(blockIndex + BlockIndex::OutwardNormal(face)) == Block::Type::Air;
}

LocalIndex Chunk::LocalIndexFromPos(const Vec3& position)
{
  return LocalIndex::ToIndex(glm::floor(position / s_ChunkLength));
}

void Chunk::BindBuffers()
{
  s_Shader->bind();
  s_TextureArray->bind(s_TextureSlot);
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



void Chunk::generateMesh()
{
  // NOTE: Should probably replace with custom memory allocation system
  static constexpr int maxVertices = 4 * 6 * TotalBlocks();
  static uint32_t* const meshData = new uint32_t[maxVertices];

  static constexpr int8_t offsets[6][4][3]
    = { { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },    /*  East Face   */
        { {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1} },    /*  West Face   */
        { {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1} },    /*  North Face  */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /*  South Face  */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} },    /*  Top Face    */
        { {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0} } };  /*  Bottom Face */

  // If chunk is empty, no need to generate mesh
  if (isEmpty())
    return;

  EN_PROFILE_FUNCTION();

  m_QuadCount = 0;
  for (blockIndex_t i = 0; i < s_ChunkSize; ++i)
    for (blockIndex_t j = 0; j < s_ChunkSize; ++j)
      for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
        if (getBlockType(i, j, k) != Block::Type::Air)
          for (Block::Face face : Block::FaceIterator())
            if (isBlockNeighborTransparent(i, j, k, face))
            {
              const int faceID = static_cast<int>(face);
              const blockTexID textureID = static_cast<blockTexID>(Block::GetTexture(getBlockType(i, j, k), face));

              for (uint8_t v = 0; v < 4; ++v)
              {
                // Add offsets to convert from block index to vertex index
                const uint8_t vI = i + offsets[faceID][v][0];
                const uint8_t vJ = j + offsets[faceID][v][1];
                const uint8_t vK = k + offsets[faceID][v][2];

                uint32_t vertexData = vI + (vJ << 6) + (vK << 12);  // Local vertex coordinates
                vertexData |= faceID << 18;                         // Face index
                vertexData |= v << 21;                              // Quad vertex index
                vertexData |= textureID << 23;                      // TextureID

                meshData[4 * m_QuadCount + v] = vertexData;
              }
              m_QuadCount++;
            }
  m_MeshState = MeshState::Simple;

  if (m_QuadCount == 0)
    return;

  m_VertexArray->setVertexBuffer(meshData, 4 * sizeof(uint32_t) * m_QuadCount);
}

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);

  EN_ASSERT(m_ChunkComposition != nullptr, "Chunk data has not yet been allocated!");
  EN_ASSERT(0 <= i && i < chunkSize && 0 <= j && j < chunkSize && 0 <= k && k < chunkSize, "Index is out of bounds!");

  m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
}

void Chunk::setBlockType(const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

void Chunk::determineOpacity()
{
  for (Block::Face face : Block::FaceIterator())
  {
    const int faceID = static_cast<int>(face);

    // Relabel coordinates
    const int u = faceID / 2;
    const int v = (u + 1) % 3;
    const int w = (u + 2) % 3;

    for (blockIndex_t i = 0; i < s_ChunkSize; ++i)
      for (blockIndex_t j = 0; j < s_ChunkSize; ++j)
      {
        BlockIndex blockIndex{};
        blockIndex[u] = faceID % 2 == 0 ? s_ChunkSize - 1 : 0;
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

void Chunk::sendAddressUpdate()
{
  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);
    if (m_Neighbors[faceID] != nullptr)
    {
      int oppFaceID = static_cast<int>(!face);
      m_Neighbors[faceID]->m_Neighbors[oppFaceID] = this; // Avoid using setNeighbor() to prevent re-meshing
    }
  }
}

void Chunk::excise()
{
  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);
    if (m_Neighbors[faceID] != nullptr)
    {
      EN_ASSERT(m_Neighbors[faceID]->getNeighbor(!face) == this, "Incorrect neighbor!");
      m_Neighbors[faceID]->setNeighbor(!face, nullptr);
    }
  }
}

void Chunk::markAsEmpty()
{
  m_ChunkComposition.reset();
  m_NonOpaqueFaces = 0xFF;
  m_MeshState = MeshState::NotGenerated;
}

BlockIndex Chunk::blockIndexFromPos(const Vec3& position) const
{
  Vec3 localPosition = position - anchorPosition();

  EN_ASSERT(localPosition.x >= 0.0 && localPosition.x <= s_ChunkLength &&
            localPosition.y >= 0.0 && localPosition.y <= s_ChunkLength &&
            localPosition.z >= 0.0 && localPosition.z <= s_ChunkLength, "Given position is not inside chunk!");

  return BlockIndex::ToIndex(localPosition / Block::Length());
}