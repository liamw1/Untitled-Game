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
}

Chunk::~Chunk()
{
  excise();
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_ChunkIndex(std::move(other.m_ChunkIndex)),
    m_ChunkComposition(std::move(other.m_ChunkComposition)),
    m_Empty(std::move(other.m_Empty)),
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
    m_Empty = std::move(other.m_Empty);
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
  m_ChunkComposition = Engine::CreateUnique<BlockType[]>(s_ChunkTotalBlocks);

  const float chunkHeight = position()[2];
  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
    {
      const float& terrainHeight = heightMap.terrainHeights[i][j];

      if (terrainHeight <= chunkHeight)
      {
        for (uint8_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, BlockType::Air);
      }
      else if (terrainHeight >= chunkHeight + s_ChunkLength)
      {
        for (uint8_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, BlockType::Grass);
        m_Empty = false;
      }
      else
      {
        const int terrainHeightIndex = static_cast<int>((terrainHeight - chunkHeight) / Block::Length());
        for (uint8_t k = 0; k < s_ChunkSize; ++k)
        {
          if (k < terrainHeightIndex)
          {
            setBlockType(i, j, k, BlockType::Grass);
            m_Empty = false;
          }
          else
            setBlockType(i, j, k, BlockType::Air);
        }
      }
    }

  if (m_Empty)
  {
    m_ChunkComposition.reset();
    for (BlockFace face : BlockFaceIterator())
      m_FaceIsOpaque[static_cast<uint8_t>(face)] = false;
  }
  else
    for (uint8_t i = 0; i < s_ChunkSize; ++i)
      for (uint8_t j = 0; j < s_ChunkSize; ++j)
      {
        if (getBlockType(s_ChunkSize - 1, i, j) == BlockType::Air)
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::East)] = false;
        if (getBlockType(0, i, j) == BlockType::Air)
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::West)] = false;
        if (getBlockType(i, s_ChunkSize - 1, j) == BlockType::Air)
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::North)] = false;
        if (getBlockType(i, 0, j) == BlockType::Air)
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::South)] = false;
        if (getBlockType(i, j, s_ChunkSize - 1) == BlockType::Air)
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::Top)] = false;
        if (getBlockType(i, j, 0) == BlockType::Air)
          m_FaceIsOpaque[static_cast<uint8_t>(BlockFace::Bottom)] = false;
      }
}

void Chunk::generateMesh()
{
  // If chunk is empty, no need to generate mesh
  if (m_Empty)
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
              for (uint8_t v = 0; v < 4; ++v)
              {
                const uint8_t faceID = static_cast<uint8_t>(face);

                // Add offsets to convert from block index to vertex index
                const uint8_t vI = i + offsets[faceID][v][0];
                const uint8_t vJ = j + offsets[faceID][v][1];
                const uint8_t vK = k + offsets[faceID][v][2];

                uint32_t vertexData = vI + (vJ << 6) + (vK << 12);
                vertexData += static_cast<uint8_t>(face) << 18;
                vertexData += v << 21;

                if (face == BlockFace::Top)
                {
                  vertexData += 6Ui8 << 23;
                  vertexData += 8Ui8 << 27;
                }
                else if (face == BlockFace::Bottom)
                {
                  vertexData += 7Ui8 << 23;
                  vertexData += 4Ui8 << 27;
                }
                else
                {
                  vertexData += 7Ui8 << 23;
                  vertexData += 5Ui8 << 27;
                }

                m_Mesh.push_back(vertexData);
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
  m_MeshVertexBuffer->bind();
  m_MeshVertexArray->bind();
}

void Chunk::reset()
{
  // Reset data to default values
  m_ChunkComposition.reset();
  m_Empty = true;
  m_FaceIsOpaque = { true, true, true, true, true, true };
  m_MeshState = MeshState::NotGenerated;
  m_Mesh.clear();
  m_MeshVertexArray.reset();
  m_MeshVertexBuffer.reset();

  // Ensure no further communication between other chunks
  excise();
  m_Neighbors = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
}

void Chunk::setBlockType(uint8_t i, uint8_t j, uint8_t k, BlockType blockType)
{
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);

  EN_ASSERT(m_ChunkComposition != nullptr, "Chunk data has not yet been allocated!");
  EN_ASSERT(i < chunkSize && j < chunkSize && k < chunkSize, "Index is out of bounds!");

  m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
}

BlockType Chunk::getBlockType(uint8_t i, uint8_t j, uint8_t k) const
{
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);

  EN_ASSERT(m_ChunkComposition != nullptr, "Chunk data has not yet been allocated!");
  EN_ASSERT(i < chunkSize && j < chunkSize && k < chunkSize, "Index is out of bounds!");

  return m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

glm::vec3 Chunk::blockPosition(uint8_t i, uint8_t j, uint8_t k) const
{
  return glm::vec3(m_ChunkIndex[0] * s_ChunkLength + i * Block::Length(),
                   m_ChunkIndex[1] * s_ChunkLength + j * Block::Length(),
                   m_ChunkIndex[2] * s_ChunkLength + k * Block::Length());
}

void Chunk::setNeighbor(BlockFace face, Chunk* chunk)
{
  m_Neighbors[static_cast<uint8_t>(face)] = chunk;
  m_MeshState = MeshState::NeedsUpdate;
}

const ChunkIndex Chunk::GetPlayerChunkIndex(const glm::vec3& playerPosition)
{
  ChunkIndex playerChunkIndex = { static_cast<int64_t>(playerPosition.x / s_ChunkLength),
                                  static_cast<int64_t>(playerPosition.y / s_ChunkLength),
                                  static_cast<int64_t>(playerPosition.z / s_ChunkLength) };
  for (int i = 0; i < 3; ++i)
    if (playerPosition[i] < 0.0f)
      playerChunkIndex[i]--;

  return playerChunkIndex;
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

    return m_Neighbors[faceID]->getBlockType(i - (s_ChunkSize - 1) * normals[faceID][0],
                                             j - (s_ChunkSize - 1) * normals[faceID][1],
                                             k - (s_ChunkSize - 1) * normals[faceID][2]) == BlockType::Air;
  }
  else
  {
    return getBlockType(i + normals[faceID][0],
                        j + normals[faceID][1],
                        k + normals[faceID][2]) == BlockType::Air;
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

      EN_ASSERT(m_Neighbors[faceID]->getNeighbor(!face) == &other, "Incorrect neighbor!");
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
