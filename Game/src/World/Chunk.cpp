#include "Chunk.h"

static constexpr glm::vec3 s_BlockFacePositions[6][4]
= {   // Top Face
    { { -s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2, },
      {  s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2, },
      {  s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2, },
      { -s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2  } },

      // Bottom Face
    { { -s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2, },
      {  s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2, },
      {  s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2, },
      { -s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2  } },

      // North Face
    { { -s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2, },
      {  s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2, },
      {  s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2, },
      { -s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2  } },

      // South Face
    { {  s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2, },
      { -s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2, },
      { -s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2, },
      {  s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2  } },

      // East Face
    { { -s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2, },
      { -s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2, },
      { -s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2, },
      { -s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2  } },

      // West Face
    { {  s_BlockSize / 2, -s_BlockSize / 2,  s_BlockSize / 2, },
      {  s_BlockSize / 2, -s_BlockSize / 2, -s_BlockSize / 2, },
      {  s_BlockSize / 2,  s_BlockSize / 2, -s_BlockSize / 2, },
      {  s_BlockSize / 2,  s_BlockSize / 2,  s_BlockSize / 2  } } };

Engine::Shared<Engine::IndexBuffer> Chunk::s_MeshIndexBuffer = nullptr;



Chunk::Chunk()
  : m_ChunkIndices({0, 0, 0}),
    m_ChunkComposition(nullptr)
{
}

Chunk::Chunk(const std::array<int64_t, 3>& chunkIndices)
  : m_ChunkIndices(chunkIndices)
{
  m_ChunkComposition = std::make_unique<Block[]>(s_ChunkTotalBlocks);
}

void Chunk::load(Block blockType)
{
  EN_PROFILE_FUNCTION();

  if (m_ChunkIndices[1] != -1)
  {
    m_ChunkComposition.reset();
    return;
  }

  constexpr uint64_t chunkSize = (uint64_t)s_ChunkSize;
  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
      {
        if (j < 16 && m_ChunkIndices[1] == -1)
          m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
        else
          m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = Block::Air;
      }

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
  EN_PROFILE_FUNCTION();

  for (uint8_t i = 0; i < 6; ++i)
  {
    EN_ASSERT(m_Neighbors[i] != nullptr, "Chunk neighbor does not exist!");
    EN_ASSERT(m_Neighbors[i]->isLoaded(), "Chunk neighbor has not been loaded yet!");
  }

  static constexpr glm::vec2 textureCoordinates[4] = { {0.0f, 0.0f},
                                                       {1.0f, 0.0f},
                                                       {1.0f, 1.0f},
                                                       {0.0f, 1.0f} };

  for (uint8_t i = 0; i < s_ChunkSize; ++i)
    for (uint8_t j = 0; j < s_ChunkSize; ++j)
      for (uint8_t k = 0; k < s_ChunkSize; ++k)
        if (getBlockAt(i, j, k) != Block::Air)
          for (uint8_t face = (uint8_t)BlockFace::Top; face <= (uint8_t)BlockFace::West; ++face)
          {
            // Check all faces for air blocks
            if (isNeighborTransparent(i, j, k, face))
            {
              glm::vec3 relativePosition = { i * s_BlockSize, j * s_BlockSize, k * s_BlockSize };

              for (int i = 0; i < 4; ++i)
                m_Mesh.push_back({ position() + relativePosition + s_BlockFacePositions[face][i], textureCoordinates[i] });
            }
          }
  m_MeshState = MeshState::Simple;

  // NOTE: Potential optimization by using reserve() for mesh vector

  generateVertexArray();
}

void Chunk::bindBuffers() const
{
  EN_PROFILE_FUNCTION();

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

const std::array<int64_t, 3> Chunk::GetPlayerChunk(const glm::vec3& playerPosition)
{
  std::array<int64_t, 3> playerChunkIndices = { (int64_t)(playerPosition.x / Chunk::Length()), (int64_t)(playerPosition.y / Chunk::Length()) , (int64_t)(playerPosition.z / Chunk::Length()) };
  for (int i = 0; i < 3; ++i)
    if (playerPosition[i] < 0.0f)
      playerChunkIndices[i]--;

  return playerChunkIndices;
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
  m_MeshVertexBuffer = Engine::VertexBuffer::Create((uint32_t)m_Mesh.size() * sizeof(BlockVertex));
  m_MeshVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                  { ShaderDataType::Float2, "a_TexCoord" } });
  m_MeshVertexArray->addVertexBuffer(m_MeshVertexBuffer);

  m_MeshVertexArray->setIndexBuffer(s_MeshIndexBuffer);

  uintptr_t dataSize = sizeof(BlockVertex) * m_Mesh.size();
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