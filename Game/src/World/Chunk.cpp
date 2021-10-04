#include "Chunk.h"

static constexpr int8_t s_Offsets[6][4][3]
= { { {0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0} },    /* Top Face    */
    { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /* Bottom Face */
    { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} },    /* North Face  */
    { {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0} },    /* South Face  */
    { {0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0} },    /* East Face   */
    { {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1} } };  /* West Face   */

Engine::Shared<Engine::IndexBuffer> Chunk::s_MeshIndexBuffer = nullptr;

Chunk::Chunk()
  : m_ChunkIndex({0, 0, 0}),
    m_ChunkComposition(nullptr)
{
}

Chunk::Chunk(const std::array<int64_t, 3>& chunkIndex)
  : m_ChunkIndex(chunkIndex)
{
  m_ChunkComposition = std::make_unique<Block[]>(s_ChunkTotalBlocks);
}

void Chunk::load(Block blockType)
{
  if (m_ChunkIndex[1] != -1)
  {
    m_ChunkComposition.reset();
    return;
  }

  constexpr uint64_t chunkSize = (uint64_t)s_ChunkSize;
  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
      {
        if (j < i / 2 + 8 && j < (s_ChunkSize - i) / 2 + 8 && j < k / 2 + 8 && j < (s_ChunkSize - k) / 2 + 8 && m_ChunkIndex[1] == -1)
          m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
        else
          m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = Block::Air;
      }

  // (i - 16) * (i - 16) + (j - 16) * (j - 16) + (k - 16) * (k - 16) < 20 * 20

  m_Empty = false;
}

void Chunk::unload()
{
  for (uint8_t face = (uint8_t)BlockFace::Top; face <= (uint8_t)BlockFace::West; ++face)
    if (m_Neighbors[face] != nullptr)
    {
      uint8_t oppFace = face % 2 == 0 ? face + 1 : face - 1;
      EN_ASSERT(m_Neighbors[face]->getNeighbor(oppFace) == this, "Incorrect neighbor!");
      m_Neighbors[face]->setNeighbor(oppFace, nullptr);
    }
}

void Chunk::generateMesh()
{
  for (uint8_t i = 0; i < 6; ++i)
    EN_ASSERT(m_Neighbors[i] != nullptr, "Chunk neighbor does not exist!");

  static constexpr glm::vec2 textureCoordinates[4] = { {0.0f, 0.0f},
                                                       {1.0f, 0.0f},
                                                       {1.0f, 1.0f},
                                                       {0.0f, 1.0f} };

  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
        if (getBlockAt(i, j, k) != Block::Air)
          for (uint8_t face = (uint8_t)BlockFace::Top; face <= (uint8_t)BlockFace::West; ++face)
            if (isNeighborTransparent(i, j, k, face))
              for (uint8_t v = 0; v < 4; ++v)
              {
                // Add offsets to convert from block index to vertex index
                const uint8_t vI = i + s_Offsets[face][v][0];
                const uint8_t vJ = j + s_Offsets[face][v][1];
                const uint8_t vK = k + s_Offsets[face][v][2];

                uint32_t vertexData = vI + (vJ << 5) + (vK << 10);
                vertexData += face << 15;
                vertexData += v << 18;
                vertexData += (uint8_t)getBlockAt(i, j, k) << 20;

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

bool Chunk::allNeighborsLoaded() const
{
  for (uint8_t face = (uint8_t)BlockFace::Top; face <= (uint8_t)BlockFace::West; ++face)
    if (m_Neighbors[face] == nullptr)
      return false;
  return true;
}

const std::array<int64_t, 3> Chunk::GetPlayerChunkIndex(const glm::vec3& playerPosition)
{
  std::array<int64_t, 3> playerChunkIndex = { (int64_t)(playerPosition.x / Chunk::Length()), (int64_t)(playerPosition.y / Chunk::Length()) , (int64_t)(playerPosition.z / Chunk::Length()) };
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

bool Chunk::isOnBoundary(uint8_t i, uint8_t j, uint8_t k, uint8_t face)
{
  using func = bool(*)(uint8_t, uint8_t, uint8_t, uint8_t);
  static const func isOnChunkSide[6] = { [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return j == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return j == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return k == s_ChunkSize - 1; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return k == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return i == 0; },
                                         [](uint8_t i, uint8_t j, uint8_t k, uint8_t face) { return i == s_ChunkSize - 1; } };

  return isOnChunkSide[face](i, j, k, face);
}

bool Chunk::isNeighborTransparent(uint8_t i, uint8_t j, uint8_t k, uint8_t face)
{
  static constexpr int8_t normals[6][3] = { { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1}, { -1, 0, 0}, { 1, 0, 0} };
                                       //       Top        Bottom       North       South         East         West

  if (isOnBoundary(i, j, k, face))
    return m_Neighbors[face]->getBlockAt(i - (s_ChunkSize - 1) * normals[face][0], j - (s_ChunkSize - 1) * normals[face][1], k - (s_ChunkSize - 1) * normals[face][2]) == Block::Air;
  else
    return getBlockAt(i + normals[face][0], j + normals[face][1], k + normals[face][2]) == Block::Air;
}