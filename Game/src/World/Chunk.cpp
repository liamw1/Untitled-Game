#include "GMpch.h"
#include "Chunk.h"

Engine::Shared<Engine::IndexBuffer> Chunk::s_MeshIndexBuffer = nullptr;

Chunk::Chunk()
  : m_ChunkIndex({0, 0, 0}),
    m_ChunkComposition(nullptr)
{
}

Chunk::Chunk(const std::array<int64_t, 3>& chunkIndex)
  : m_ChunkIndex(chunkIndex),
    m_ChunkComposition(nullptr)
{
}

Chunk::~Chunk()
{
  for (BlockFace face : BlockFaceIterator())
    if (m_Neighbors[static_cast<uint8_t>(face)] != nullptr)
    {
      EN_ASSERT(m_Neighbors[static_cast<uint8_t>(face)]->getNeighbor(!face) == this, "Incorrect neighbor!");
      m_Neighbors[static_cast<uint8_t>(face)]->setNeighbor(!face, nullptr);
    }
}

void Chunk::load(Block blockType)
{
  m_ChunkComposition = std::make_unique<Block[]>(s_ChunkTotalBlocks);

  constexpr uint64_t chunkSize = (uint64_t)s_ChunkSize;
  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t k = 0; k < s_ChunkSize; ++k)
    {
      glm::vec2 blockXZ = glm::vec2(m_ChunkIndex[0] * s_ChunkLength + i * s_BlockSize, m_ChunkIndex[2] * s_ChunkLength + k * s_BlockSize);
      float terrainHeight = 20.0f * glm::simplex(blockXZ / 32.0f);

      if (terrainHeight < position()[1])
        for (uint8_t j = 0; j < s_ChunkSize; ++j)
          m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = Block::Air;
      else if (terrainHeight > position()[1] + s_ChunkLength)
        for (uint8_t j = 0; j < s_ChunkSize; ++j)
          m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
      else
      {
        for (int j = (int)(terrainHeight - position()[1]); j >= 0; --j)
          m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
      }
    }

  m_Empty = false;
}

void Chunk::generateMesh()
{
  for (uint8_t i = 0; i < 6; ++i)
    EN_ASSERT(m_Neighbors[i] != nullptr, "Chunk neighbor does not exist!");

  static constexpr int8_t offsets[6][4][3]
    = { { {0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0} },    /* Top Face    */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /* Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} },    /* North Face  */
        { {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0} },    /* South Face  */
        { {0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0} },    /* East Face   */
        { {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1} } };  /* West Face   */

  static constexpr glm::vec2 textureCoordinates[4] = { {0.0f, 0.0f},
                                                       {1.0f, 0.0f},
                                                       {1.0f, 1.0f},
                                                       {0.0f, 1.0f} };

  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
        if (getBlockAt(i, j, k) != Block::Air)
          for (BlockFace face : BlockFaceIterator())
            if (isBlockNeighborTransparent(i, j, k, face))
              for (uint8_t v = 0; v < 4; ++v)
              {
                // Add offsets to convert from block index to vertex index
                const uint8_t vI = i + offsets[static_cast<uint8_t>(face)][v][0];
                const uint8_t vJ = j + offsets[static_cast<uint8_t>(face)][v][1];
                const uint8_t vK = k + offsets[static_cast<uint8_t>(face)][v][2];

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

void Chunk::bindBuffers() const
{
  m_MeshVertexBuffer->bind();
  m_MeshVertexArray->bind();
}

Block Chunk::getBlockAt(uint8_t i, uint8_t j, uint8_t k) const
{
  static constexpr uint64_t chunkSize = (uint64_t)s_ChunkSize;
  return m_Empty ? Block::Air : m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

glm::vec3 Chunk::blockPosition(uint8_t i, uint8_t j, uint8_t k) const
{
  return glm::vec3(m_ChunkIndex[0] * s_ChunkLength + i * s_BlockSize,
                   m_ChunkIndex[1] * s_ChunkLength + j * s_BlockSize,
                   m_ChunkIndex[2] * s_ChunkLength + k * s_BlockSize);
}

bool Chunk::allNeighborsLoaded() const
{
  for (BlockFace face : BlockFaceIterator())
    if (m_Neighbors[static_cast<uint8_t>(face)] == nullptr)
      return false;
  return true;
}

const std::array<int64_t, 3> Chunk::GetPlayerChunkIndex(const glm::vec3& playerPosition)
{
  std::array<int64_t, 3> playerChunkIndex = { (int64_t)(playerPosition.x / s_ChunkLength), (int64_t)(playerPosition.y / s_ChunkLength) , (int64_t)(playerPosition.z / s_ChunkLength) };
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
  m_MeshVertexBuffer = Engine::VertexBuffer::Create((uint32_t)m_Mesh.size() * sizeof(uint32_t));
  m_MeshVertexBuffer->setLayout({ { ShaderDataType::Uint32, "a_VertexData" } });
  m_MeshVertexArray->addVertexBuffer(m_MeshVertexBuffer);

  m_MeshVertexArray->setIndexBuffer(s_MeshIndexBuffer);

  uintptr_t dataSize = sizeof(uint32_t) * m_Mesh.size();
  m_MeshVertexBuffer->setData(m_Mesh.data(), dataSize);
}

bool Chunk::isBlockOnBoundary(uint8_t i, uint8_t j, uint8_t k, BlockFace face)
{
  using func = bool(*)(uint8_t, uint8_t, uint8_t, BlockFace);
  static const func isOnChunkSide[6] = { [](uint8_t i, uint8_t j, uint8_t k, BlockFace face) { return j == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k, BlockFace face) { return j == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, BlockFace face) { return k == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k, BlockFace face) { return k == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, BlockFace face) { return i == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, BlockFace face) { return i == s_ChunkSize - 1; } };

  return isOnChunkSide[static_cast<uint8_t>(face)](i, j, k, face);
}

bool Chunk::isBlockNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, BlockFace face)
{
  static constexpr int8_t normals[6][3] = { { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1}, { -1, 0, 0}, { 1, 0, 0} };
                                       //       Top        Bottom       North       South         East         West

  if (isBlockOnBoundary(i, j, k, face))
    return m_Neighbors[(uint8_t)face]->getBlockAt(i - (s_ChunkSize - 1) * normals[static_cast<uint8_t>(face)][0],
                                                  j - (s_ChunkSize - 1) * normals[static_cast<uint8_t>(face)][1],
                                                  k - (s_ChunkSize - 1) * normals[static_cast<uint8_t>(face)][2]) == Block::Air;
  else
    return getBlockAt(i + normals[static_cast<uint8_t>(face)][0],
                      j + normals[static_cast<uint8_t>(face)][1],
                      k + normals[static_cast<uint8_t>(face)][2]) == Block::Air;
}