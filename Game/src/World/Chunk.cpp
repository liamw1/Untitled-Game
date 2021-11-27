#include "GMpch.h"
#include "Chunk.h"

Engine::Shared<Engine::IndexBuffer> Chunk::s_MeshIndexBuffer = nullptr;

Chunk::Chunk()
  : m_ChunkIndex({0, 0, 0})
{
}

Chunk::Chunk(const ChunkIndex& chunkIndex)
  : m_ChunkIndex(chunkIndex)
{
  if (s_MeshIndexBuffer == nullptr)
    InitializeIndexBuffer();
}

Chunk::~Chunk()
{
  excise();
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_ChunkIndex(std::move(other.m_ChunkIndex)),
    m_ChunkComposition(std::move(other.m_ChunkComposition)),
    m_FaceIsOpaque(std::move(other.m_FaceIsOpaque)),
    m_MeshState(std::move(other.m_MeshState)),
    m_Mesh(std::move(other.m_Mesh)),
    m_MeshVertexArray(std::move(other.m_MeshVertexArray)),
    m_MeshVertexBuffer(std::move(other.m_MeshVertexBuffer))
{
  m_Neighbors = other.m_Neighbors;
  for (BlockFace face : BlockFaceIterator())
    other.m_Neighbors[static_cast<uint8_t>(face)] = nullptr;

  // Since the address of 'other' has moved, we must update its neighbors
  sendAddressUpdate();
}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_ChunkIndex = std::move(other.m_ChunkIndex);
    m_ChunkComposition = std::move(other.m_ChunkComposition);
    m_FaceIsOpaque = std::move(other.m_FaceIsOpaque);
    m_MeshState = std::move(other.m_MeshState);
    m_Mesh = std::move(other.m_Mesh);
    m_MeshVertexArray = std::move(other.m_MeshVertexArray);
    m_MeshVertexBuffer = std::move(other.m_MeshVertexBuffer);
   
    m_Neighbors = other.m_Neighbors;
    for (BlockFace face : BlockFaceIterator())
      other.m_Neighbors[static_cast<uint8_t>(face)] = nullptr;

    // Since the address of 'other' has moved, we must update its neighbors
    sendAddressUpdate();
  }
  return *this;
}

void Chunk::load(HeightMap heightMap)
{
  const float chunkHeight = anchorPoint()[2];
  m_ChunkComposition = Engine::CreateUnique<BlockType[]>(s_ChunkTotalBlocks);

  bool isEmpty = true;
  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
    {
      const float& terrainHeight = heightMap.terrainHeights[i][j];

      if (terrainHeight < chunkHeight)
      {
        for (uint8_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, BlockType::Air);
      }
      else if (terrainHeight > chunkHeight + s_ChunkLength + Block::Length())
      {
        for (uint8_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, BlockType::Dirt);
        isEmpty = false;
      }
      else
      {
        const int terrainHeightIndex = static_cast<int>((terrainHeight - chunkHeight) / Block::Length());
        for (uint8_t k = 0; k < s_ChunkSize; ++k)
        {
          if (k == terrainHeightIndex)
          {
            setBlockType(i, j, k, BlockType::Grass);
            isEmpty = false;
          }
          else if (k < terrainHeightIndex)
          {
            setBlockType(i, j, k, BlockType::Dirt);
            isEmpty = false;
          }
          else
            setBlockType(i, j, k, BlockType::Air);
        }
      }
    }

  if (isEmpty)
  {
    m_ChunkComposition.reset();
    for (BlockFace face : BlockFaceIterator())
      m_FaceIsOpaque[static_cast<uint8_t>(face)] = false;
  }
  else
    for (uint8_t i = 0; i < s_ChunkSize; ++i)
      for (uint8_t j = 0; j < s_ChunkSize; ++j)
      {
        if (Block::HasTransparency(getBlockType(s_ChunkSize - 1, i, j)))
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::East)] = false;
        if (Block::HasTransparency(getBlockType(0, i, j)))
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::West)] = false;
        if (Block::HasTransparency(getBlockType(i, s_ChunkSize - 1, j)))
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::North)] = false;
        if (Block::HasTransparency(getBlockType(i, 0, j)))
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::South)] = false;
        if (Block::HasTransparency(getBlockType(i, j, s_ChunkSize - 1)))
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::Top)] = false;
        if (Block::HasTransparency(getBlockType(i, j, 0)))
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::Bottom)] = false;
      }
}

void Chunk::generateMesh()
{
  // If chunk is empty, no need to generate mesh
  if (isEmpty())
    return;

  m_Mesh.clear();

  static constexpr int8_t offsets[6][4][3]
    = { { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },    /* East Face   */
        { {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1} },    /* West Face   */
        { {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1} },    /* North Face  */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /* South Face  */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} },    /* Top Face    */
        { {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0} } };  /* Bottom Face */

  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
        if (getBlockType(i, j, k) != BlockType::Air)
          for (BlockFace face : BlockFaceIterator())
            if (isBlockNeighborTransparent(i, j, k, face))
            {
              const uint8_t faceID = static_cast<uint8_t>(face);
              const blockTexID textureID = static_cast<blockTexID>(Block::GetTexture(getBlockType(i, j, k), face));

              for (uint8_t v = 0; v < 4; ++v)
              {
                // Add offsets to convert from block index to vertex index
                const uint8_t vI = i + offsets[faceID][v][0];
                const uint8_t vJ = j + offsets[faceID][v][1];
                const uint8_t vK = k + offsets[faceID][v][2];

                uint32_t vertexData = vI + (vJ << 6) + (vK << 12);  // Local vertex coordinates
                vertexData += faceID << 18;                         // Face index
                vertexData += v << 21;                              // Quad vertex index
                vertexData += textureID << 23;                      // TextureID

                m_Mesh.push_back(vertexData);
              }
            }
  m_MeshState = MeshState::Simple;

  // NOTE: Potential optimization by using reserve() for mesh vector

  generateVertexArray();
}

void Chunk::update()
{
  if (m_MeshState == MeshState::NotGenerated || m_MeshState == MeshState::NeedsUpdate)
    generateMesh();
}

void Chunk::bindBuffers() const
{
  m_MeshVertexArray->bind();
}

void Chunk::reset()
{
  // Reset data to default values
  m_ChunkComposition.reset();
  m_FaceIsOpaque = { true, true, true, true, true, true };
  m_MeshState = MeshState::NotGenerated;
  m_Mesh.clear();
  m_MeshVertexArray.reset();
  m_MeshVertexBuffer.reset();

  // Ensure no further communication between other chunks
  excise();
  m_Neighbors = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
}

BlockType Chunk::getBlockType(uint8_t i, uint8_t j, uint8_t k) const
{
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);
  EN_ASSERT(i < chunkSize && j < chunkSize && k < chunkSize, "Index is out of bounds!");
  return isEmpty() ? BlockType::Air : m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

BlockType Chunk::getBlockType(const LocalIndex& blockIndex) const
{
  return getBlockType(blockIndex[0], blockIndex[1], blockIndex[2]);
}

BlockType Chunk::getBlockType(const glm::vec3& position) const
{
  return getBlockType(blockIndexFromPos(position));
}

void Chunk::setBlockType(uint8_t i, uint8_t j, uint8_t k, BlockType blockType)
{
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);

  EN_ASSERT(m_ChunkComposition != nullptr, "Chunk data has not yet been allocated!");
  EN_ASSERT(i < chunkSize && j < chunkSize && k < chunkSize, "Index is out of bounds!");

  m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
}

void Chunk::setBlockType(const LocalIndex& blockIndex, BlockType blockType)
{
  setBlockType(blockIndex[0], blockIndex[1], blockIndex[2], blockType);
}

void Chunk::setNeighbor(BlockFace face, Chunk* chunk)
{
  m_Neighbors[static_cast<uint8_t>(face)] = chunk;
  m_MeshState = MeshState::NeedsUpdate;
}

const ChunkIndex Chunk::ChunkIndexFromPos(const glm::vec3& position)
{
  ChunkIndex chunkIndex = { static_cast<int64_t>(position.x / s_ChunkLength),
                            static_cast<int64_t>(position.y / s_ChunkLength),
                            static_cast<int64_t>(position.z / s_ChunkLength) };
  for (int i = 0; i < 3; ++i)
    if (position[i] < 0.0f)
      chunkIndex[i]--;

  return chunkIndex;
}

void Chunk::InitializeIndexBuffer()
{
  constexpr uint32_t maxIndices = 6 * 3 * TotalBlocks();

  uint32_t* meshIndices = new uint32_t[maxIndices];

  uint32_t offset = 0;
  for (uint32_t i = 0; i < maxIndices; i += 6)
  {
    // Triangle 1
    meshIndices[i + 0] = offset + 0;
    meshIndices[i + 1] = offset + 1;
    meshIndices[i + 2] = offset + 2;

    // Triangle 2
    meshIndices[i + 3] = offset + 2;
    meshIndices[i + 4] = offset + 3;
    meshIndices[i + 5] = offset + 0;

    offset += 4;
  }
  s_MeshIndexBuffer = Engine::IndexBuffer::Create(meshIndices, maxIndices);

  delete[] meshIndices;
}

void Chunk::generateVertexArray()
{
  m_MeshVertexArray = Engine::VertexArray::Create();
  m_MeshVertexBuffer = Engine::VertexBuffer::Create(static_cast<uint32_t>(m_Mesh.size()) * sizeof(uint32_t));
  m_MeshVertexBuffer->setLayout({ { ShaderDataType::Uint32, "a_VertexData" } });
  m_MeshVertexArray->addVertexBuffer(m_MeshVertexBuffer);

  m_MeshVertexArray->setIndexBuffer(s_MeshIndexBuffer);

  uintptr_t dataSize = sizeof(uint32_t) * m_Mesh.size();
  m_MeshVertexBuffer->setData(m_Mesh.data(), dataSize);
}

bool Chunk::isBlockNeighborInAnotherChunk(uint8_t i, uint8_t j, uint8_t k, BlockFace face)
{
  using func = bool(*)(uint8_t, uint8_t, uint8_t);
  static const func isOnChunkSide[6] = { [](uint8_t i, uint8_t j, uint8_t k) { return i == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k) { return i == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k) { return j == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k) { return j == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k) { return k == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k) { return k == 0; } };

  return isOnChunkSide[static_cast<uint8_t>(face)](i, j, k);
}

bool Chunk::isBlockNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, BlockFace face)
{
  static constexpr int8_t normals[6][3] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                       //      East         West        North       South         Top        Bottom

  const uint8_t faceID = static_cast<uint8_t>(face);
  if (isBlockNeighborInAnotherChunk(i, j, k, face))
  {
    if (m_Neighbors[faceID] == nullptr)
      return false;
    else if (m_Neighbors[faceID]->isEmpty())
      return true;

    return Block::HasTransparency(m_Neighbors[faceID]->getBlockType(i - (s_ChunkSize - 1) * normals[faceID][0],
                                                                    j - (s_ChunkSize - 1) * normals[faceID][1],
                                                                    k - (s_ChunkSize - 1) * normals[faceID][2]));
  }
  else
  {
    return Block::HasTransparency(getBlockType(i + normals[faceID][0],
                                               j + normals[faceID][1],
                                               k + normals[faceID][2]));
  }
}

void Chunk::sendAddressUpdate()
{
  for (BlockFace face : BlockFaceIterator())
  {
    uint8_t faceID = static_cast<uint8_t>(face);
    if (m_Neighbors[faceID] != nullptr)
    {
      uint8_t oppFaceID = static_cast<uint8_t>(!face);
      m_Neighbors[faceID]->m_Neighbors[oppFaceID] = this; // Avoid using setNeighbor() to prevent re-meshing
    }
  }
}

void Chunk::excise()
{
  for (BlockFace face : BlockFaceIterator())
  {
    uint8_t faceID = static_cast<uint8_t>(face);
    if (m_Neighbors[faceID] != nullptr)
    {
      EN_ASSERT(m_Neighbors[faceID]->getNeighbor(!face) == this, "Incorrect neighbor!");
      m_Neighbors[faceID]->setNeighbor(!face, nullptr);
    }
  }
}

LocalIndex Chunk::blockIndexFromPos(const glm::vec3& position) const
{
  glm::vec3 localPosition = position - anchorPoint();

  EN_ASSERT(localPosition.x >= 0.0f && localPosition.y >= 0.0f && localPosition.z >= 0.0f, "Given position is not inside chunk!");

  LocalIndex blockIndex = { static_cast<uint8_t>(localPosition.x / Block::Length()),
                            static_cast<uint8_t>(localPosition.y / Block::Length()),
                            static_cast<uint8_t>(localPosition.z / Block::Length()) };
  return blockIndex;
}
