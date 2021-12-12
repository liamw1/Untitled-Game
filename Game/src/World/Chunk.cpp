#include "GMpch.h"
#include "Chunk.h"
#include "Player/Player.h"
#include "Util/MarchingCubes.h"

Engine::Shared<Engine::IndexBuffer> Chunk::s_MeshIndexBuffer = nullptr;

Chunk::Chunk()
  : m_GlobalIndex({ 0, 0, 0 })
{
}

Chunk::Chunk(const GlobalIndex& chunkIndex)
  : m_GlobalIndex(chunkIndex)
{
  if (s_MeshIndexBuffer == nullptr)
    InitializeIndexBuffer();
}

Chunk::~Chunk()
{
  excise();
}

Chunk::Chunk(Chunk&& other) noexcept
  : m_GlobalIndex(std::move(other.m_GlobalIndex)),
  m_ChunkComposition(std::move(other.m_ChunkComposition)),
  m_FaceIsOpaque(std::move(other.m_FaceIsOpaque)),
  m_MeshState(std::move(other.m_MeshState)),
  m_Mesh(std::move(other.m_Mesh)),
  m_MeshVertexArray(std::move(other.m_MeshVertexArray)),
  m_MeshVertexBuffer(std::move(other.m_MeshVertexBuffer))
{
  // Transfer neighbor pointers
  m_Neighbors = other.m_Neighbors;
  for (BlockFace face : BlockFaceIterator())
    other.m_Neighbors[static_cast<uint8_t>(face)] = nullptr;

  // Since the address of 'other' has moved, we must update its neighbors about the change
  sendAddressUpdate();
}

Chunk& Chunk::operator=(Chunk&& other) noexcept
{
  if (this != &other)
  {
    m_GlobalIndex = std::move(other.m_GlobalIndex);
    m_ChunkComposition = std::move(other.m_ChunkComposition);
    m_FaceIsOpaque = std::move(other.m_FaceIsOpaque);
    m_MeshState = std::move(other.m_MeshState);
    m_Mesh = std::move(other.m_Mesh);
    m_MeshVertexArray = std::move(other.m_MeshVertexArray);
    m_MeshVertexBuffer = std::move(other.m_MeshVertexBuffer);

    // Transfer neighbor pointers
    m_Neighbors = other.m_Neighbors;
    for (BlockFace face : BlockFaceIterator())
      other.m_Neighbors[static_cast<uint8_t>(face)] = nullptr;

    // Since the address of 'other' has moved, we must update its neighbors about the change
    sendAddressUpdate();
  }
  return *this;
}

void Chunk::load(HeightMap heightMap)
{
  const length_t chunkHeight = s_ChunkLength * getGlobalIndex().k;

  if (heightMap.maxHeight < chunkHeight)
  {
    markAsEmpty();
    return;
  }

  m_ChunkComposition = Engine::CreateUnique<BlockType[]>(s_ChunkTotalBlocks);

  bool isEmpty = true;
  for (blockIndex_t i = 0; i < s_ChunkSize; ++i)
    for (blockIndex_t j = 0; j < s_ChunkSize; ++j)
    {
      const length_t& terrainHeight = heightMap.terrainHeights[i][j];

      if (terrainHeight < chunkHeight)
      {
        for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, BlockType::Air);
      }
      else if (terrainHeight > chunkHeight + s_ChunkLength + Block::Length())
      {
        for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
          setBlockType(i, j, k, BlockType::Dirt);
        isEmpty = false;
      }
      else
      {
        const int terrainHeightIndex = static_cast<int>((terrainHeight - chunkHeight) / Block::Length());
        for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
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

  if (m_ChunkComposition[0] == BlockType::Air && m_Mesh.size() == 0)
    markAsEmpty();
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

LocalIndex Chunk::getLocalIndex() const
{
  return CalcRelativeIndex(m_GlobalIndex, Player::OriginIndex());
}

Vec3 Chunk::anchorPoint() const
{
  const LocalIndex localIndex = getLocalIndex();
  return s_ChunkLength * Vec3(localIndex.i, localIndex.j, localIndex.k);
}

BlockType Chunk::getBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k) const
{
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);
  EN_ASSERT(i < chunkSize&& j < chunkSize&& k < chunkSize, "Index is out of bounds!");
  return isEmpty() ? BlockType::Air : m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k];
}

BlockType Chunk::getBlockType(const BlockIndex& blockIndex) const
{
  return getBlockType(blockIndex.i, blockIndex.j, blockIndex.k);
}

BlockType Chunk::getBlockType(const Vec3& position) const
{
  return getBlockType(blockIndexFromPos(position));
}

void Chunk::removeBlock(const BlockIndex& blockIndex)
{
  setBlockType(blockIndex, BlockType::Air);

  m_MeshState = MeshState::NeedsUpdate;
  for (BlockFace face : BlockFaceIterator())
  {
    const uint8_t faceID = static_cast<uint8_t>(face);

    if (isBlockNeighborInAnotherChunk(blockIndex, face) && m_Neighbors[faceID] != nullptr)
      m_Neighbors[faceID]->m_MeshState = MeshState::NeedsUpdate;
  }
}

void Chunk::placeBlock(const BlockIndex& blockIndex, BlockFace face, BlockType blockType)
{
  static constexpr BlockIndex normals[6] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
  //      East         West        North       South         Top        Bottom

  EN_ASSERT(!isEmpty(), "Place block cannot be called on an empty chunk!");

  // If trying to place an air block or trying to place a block in a space occupied by another block, do nothing
  if (blockType == BlockType::Air || !isBlockNeighborAir(blockIndex, face))
    return;

  const uint8_t faceID = static_cast<uint8_t>(face);
  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    if (m_Neighbors[faceID] == nullptr)
      return;
    else if (m_Neighbors[faceID]->isEmpty())
      m_Neighbors[faceID]->m_ChunkComposition = Engine::CreateUnique<BlockType[]>(s_ChunkTotalBlocks);

    m_Neighbors[faceID]->setBlockType(blockIndex - static_cast<blockIndex_t>(s_ChunkSize - 1) * normals[faceID], blockType);
    m_Neighbors[faceID]->m_MeshState = MeshState::NeedsUpdate;
  }
  else
    setBlockType(blockIndex + normals[faceID], blockType);
  m_MeshState = MeshState::NeedsUpdate;
}

void Chunk::setNeighbor(BlockFace face, Chunk* chunk)
{
  m_Neighbors[static_cast<uint8_t>(face)] = chunk;
  m_MeshState = MeshState::NeedsUpdate;
}

bool Chunk::isBlockNeighborInAnotherChunk(const BlockIndex& blockIndex, BlockFace face)
{
  static constexpr blockIndex_t chunkLimits[2] = { s_ChunkSize - 1, 0 };
  const uint8_t faceID = static_cast<uint8_t>(face);
  const uint8_t coordID = faceID / 2;

  return blockIndex[coordID] == chunkLimits[faceID % 2];
}

bool Chunk::isBlockNeighborTransparent(blockIndex_t i, blockIndex_t j, blockIndex_t k, BlockFace face)
{
  return isBlockNeighborTransparent({ i, j, k }, face);
}

bool Chunk::isBlockNeighborTransparent(const BlockIndex& blockIndex, BlockFace face)
{
  static constexpr BlockIndex normals[6] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
  //      East         West        North       South         Top        Bottom

  const uint8_t faceID = static_cast<uint8_t>(face);
  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    if (m_Neighbors[faceID] == nullptr)
      return false;
    else if (m_Neighbors[faceID]->isEmpty())
      return true;

    return Block::HasTransparency(m_Neighbors[faceID]->getBlockType(blockIndex - static_cast<blockIndex_t>(s_ChunkSize - 1) * normals[faceID]));
  }
  else
    return Block::HasTransparency(getBlockType(blockIndex + normals[faceID]));
}

bool Chunk::isBlockNeighborAir(const BlockIndex& blockIndex, BlockFace face)
{
  static constexpr BlockIndex normals[6] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
  //      East         West        North       South         Top        Bottom

  const uint8_t faceID = static_cast<uint8_t>(face);
  if (isBlockNeighborInAnotherChunk(blockIndex, face))
  {
    if (m_Neighbors[faceID] == nullptr)
      return false;
    else if (m_Neighbors[faceID]->isEmpty())
      return true;

    return m_Neighbors[faceID]->getBlockType(blockIndex - static_cast<blockIndex_t>(s_ChunkSize - 1) * normals[faceID]) == BlockType::Air;
  }
  else
    return getBlockType(blockIndex + normals[faceID]) == BlockType::Air;
}

const LocalIndex Chunk::LocalIndexFromPos(const Vec3& position)
{
  return { static_cast<localIndex_t>(floor(position.x / s_ChunkLength)),
           static_cast<localIndex_t>(floor(position.y / s_ChunkLength)),
           static_cast<localIndex_t>(floor(position.z / s_ChunkLength)) };
}

void Chunk::InitializeIndexBuffer()
{
  static constexpr uint32_t maxIndices = 6 * 3 * TotalBlocks();

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

  for (blockIndex_t i = 0; i < s_ChunkSize; ++i)
    for (blockIndex_t j = 0; j < s_ChunkSize; ++j)
      for (blockIndex_t k = 0; k < s_ChunkSize; ++k)
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
                vertexData |= faceID << 18;                         // Face index
                vertexData |= v << 21;                              // Quad vertex index
                vertexData |= textureID << 23;                      // TextureID

                m_Mesh.push_back(vertexData);
              }
            }
  m_MeshState = MeshState::Simple;

  // NOTE: Potential optimization by using reserve() for mesh vector

  generateVertexArray();
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

void Chunk::setBlockType(blockIndex_t i, blockIndex_t j, blockIndex_t k, BlockType blockType)
{
  static constexpr uint64_t chunkSize = static_cast<uint64_t>(s_ChunkSize);

  EN_ASSERT(m_ChunkComposition != nullptr, "Chunk data has not yet been allocated!");
  EN_ASSERT(i < chunkSize&& j < chunkSize&& k < chunkSize, "Index is out of bounds!");

  m_ChunkComposition[i * chunkSize * chunkSize + j * chunkSize + k] = blockType;
}

void Chunk::setBlockType(const BlockIndex& blockIndex, BlockType blockType)
{
  setBlockType(blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

void Chunk::determineOpacity()
{
  for (BlockFace face : BlockFaceIterator())
  {
    const uint8_t faceID = static_cast<uint8_t>(face);

    // Relabel coordinates
    const uint8_t u = faceID / 2;
    const uint8_t v = (u + 1) % 3;
    const uint8_t w = (u + 2) % 3;

    for (blockIndex_t i = 0; i < s_ChunkSize; ++i)
      for (blockIndex_t j = 0; j < s_ChunkSize; ++j)
      {
        BlockIndex blockIndex{};
        blockIndex[u] = faceID % 2 == 0 ? s_ChunkSize - 1 : 0;
        blockIndex[v] = i;
        blockIndex[w] = j;

        if (Block::HasTransparency(getBlockType(blockIndex)))
        {
          m_FaceIsOpaque[faceID] = false;
          goto nextFace;
        }
      }
  nextFace:;
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

void Chunk::markAsEmpty()
{
  m_ChunkComposition.reset();
  for (BlockFace face : BlockFaceIterator())
    m_FaceIsOpaque[static_cast<uint8_t>(face)] = false;
  m_MeshState = MeshState::NotGenerated;
}

BlockIndex Chunk::blockIndexFromPos(const Vec3& position) const
{
  Vec3 localPosition = position - anchorPoint();

  EN_ASSERT(localPosition.x >= 0.0 && localPosition.x <= s_ChunkLength &&
    localPosition.y >= 0.0 && localPosition.y <= s_ChunkLength &&
    localPosition.z >= 0.0 && localPosition.z <= s_ChunkLength, "Given position is not inside chunk!");

  BlockIndex blockIndex = { static_cast<blockIndex_t>(localPosition.x / Block::Length()),
                            static_cast<blockIndex_t>(localPosition.y / Block::Length()),
                            static_cast<blockIndex_t>(localPosition.z / Block::Length()) };
  return blockIndex;
}