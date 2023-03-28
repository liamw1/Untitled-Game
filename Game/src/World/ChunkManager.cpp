#include "GMpch.h"
#include "ChunkManager.h"
#include "Player/Player.h"
#include "Terrain.h"
#include "Util/Util.h"

ChunkManager::ChunkManager()
  : m_Running(true),
    m_LoadMode(LoadMode::NotSet),
    m_PrevPlayerOriginIndex(Player::OriginIndex())
{
}

ChunkManager::~ChunkManager()
{
  m_Running.store(false);
  if (m_LoadThread.joinable())
    m_LoadThread.join();
  if (m_UpdateThread.joinable())
    m_UpdateThread.join();
}

void ChunkManager::initialize()
{
  m_ChunkContainer.initialize();
}

void ChunkManager::render()
{
  EN_PROFILE_FUNCTION();

  const Mat4& viewProjection = Engine::Scene::ActiveCameraViewProjection();
  std::array<Vec4, 6> frustumPlanes = Util::CalculateViewFrustumPlanes(viewProjection);

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
  static constexpr length_t chunkSphereRadius = Constants::SQRT3 * Chunk::Length() / 2;
  for (Vec4& plane : frustumPlanes)
  {
    length_t planeNormalMag = glm::length(Vec3(plane));
    plane.w += chunkSphereRadius * planeNormalMag;
  }

  // Render chunks in view frustum
  Chunk::BindBuffers();
  m_ChunkContainer.forEach(ChunkType::Renderable, [&](Chunk& chunk)
    {
      if (Util::IsInRangeOfPlayer(chunk, c_RenderDistance) && Util::IsInFrustum(chunk.center(), frustumPlanes))
      {
        std::lock_guard lock = chunk.acquireLock();

        chunk.uploadMesh();
        chunk.draw();
      }
    });
}

void ChunkManager::update()
{
  EN_PROFILE_FUNCTION();

  std::optional<GlobalIndex> updateIndex = m_ChunkContainer.getForceUpdateIndex();
  while (updateIndex)
  {
    if (!m_ChunkContainer.contains(*updateIndex))
      generateNewChunk(*updateIndex);

    std::vector<uint32_t> mesh = createMesh(*updateIndex);
    m_ChunkContainer.update(*updateIndex, mesh);

    updateIndex = m_ChunkContainer.getForceUpdateIndex();
  }
}

void ChunkManager::clean()
{
  EN_PROFILE_FUNCTION();

  if (m_PrevPlayerOriginIndex == Player::OriginIndex())
    return;
  m_PrevPlayerOriginIndex = Player::OriginIndex();

  std::vector<GlobalIndex> chunksMarkedForDeletion = m_ChunkContainer.findAll(ChunkType::Boundary, [](const Chunk& chunk)
    {
      return !Util::IsInRangeOfPlayer(chunk, c_UnloadDistance);
    });

  if (!chunksMarkedForDeletion.empty())
  {
    for (const GlobalIndex& chunkIndex : chunksMarkedForDeletion)
      if (!Util::IsInRangeOfPlayer(chunkIndex, c_UnloadDistance))
        m_ChunkContainer.erase(chunkIndex);
    Terrain::Clean(c_UnloadDistance);
  }
}

std::pair<const Chunk*, std::unique_lock<std::mutex>> ChunkManager::acquireChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return m_ChunkContainer.acquireChunk(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void ChunkManager::placeBlock(const GlobalIndex& chunkIndex, BlockIndex blockIndex, Block::Face face, Block::Type blockType)
{
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);

    if (!chunk || chunk->getBlockType(blockIndex) == Block::Type::Air)
    {
      EN_WARN("Cannot call placeBlock on an air block!");
      return;
    }

    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, face))
    {
      auto [neighbor, neighborLock] = m_ChunkContainer.acquireChunk(chunk->getGlobalIndex() + GlobalIndex::Dir(face));

      if (!neighbor)
        return;

      chunk = neighbor;
      lock = std::move(neighborLock);

      blockIndex -= static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::Dir(face);
    }
    else
      blockIndex += BlockIndex::Dir(face);

    // If trying to place an air block or trying to place a block in a space occupied by another block, do nothing
    if (blockType == Block::Type::Air || (!chunk->empty() && chunk->getBlockType(blockIndex) != Block::Type::Air))
    {
      EN_WARN("Invalid block placement!");
      return;
    }

    chunk->setBlockType(blockIndex, blockType);
  }

  m_ChunkContainer.sendBlockUpdate(chunkIndex, blockIndex);
}

void ChunkManager::removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    chunk->setBlockType(blockIndex, Block::Type::Air);
  }

  m_ChunkContainer.sendBlockUpdate(chunkIndex, blockIndex);
}

void ChunkManager::loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType)
{
  std::unique_ptr<Block::Type[]> composition = std::make_unique_for_overwrite<Block::Type[]>(Chunk::TotalBlocks());
  for (int i = 0; i < Chunk::TotalBlocks(); ++i)
    composition[i] = blockType;

  Chunk newChunk(chunkIndex);
  newChunk.setData(std::move(composition));
  m_ChunkContainer.insert(std::move(newChunk));
}



void ChunkManager::loadWorker()
{
  using namespace std::chrono_literals;

  while (m_Running.load())
  {
    EN_PROFILE_SCOPE("Load worker");

    std::unordered_set<GlobalIndex> newChunkIndices = m_ChunkContainer.findAllLoadableIndices();

    // Load First chunk if none exist
    if (newChunkIndices.empty() && m_ChunkContainer.empty())
      newChunkIndices.insert(Player::OriginIndex());

    if (!newChunkIndices.empty())
      for (const GlobalIndex& newChunkIndex : newChunkIndices)
        generateNewChunk(newChunkIndex);
    else
      std::this_thread::sleep_for(100ms);
  }
}

void ChunkManager::updateWorker()
{
  using namespace std::chrono_literals;

  while (m_Running.load())
  {
    EN_PROFILE_SCOPE("Update worker");

    std::optional<GlobalIndex> updateIndex = m_ChunkContainer.getLazyUpdateIndex();
    if (updateIndex)
    {
      std::vector<uint32_t> mesh = createMesh(*updateIndex);
      m_ChunkContainer.update(*updateIndex, mesh);
    }
    else
      std::this_thread::sleep_for(100ms);
  }
}

void ChunkManager::generateNewChunk(const GlobalIndex& chunkIndex)
{
  Chunk chunk(chunkIndex);
  switch (m_LoadMode)
  {
    case ChunkManager::NotSet:  EN_ERROR("Load mode not set!");               break;
    case ChunkManager::Void:    chunk = Terrain::GenerateEmpty(chunkIndex);   break;
    case ChunkManager::Terrain: chunk = Terrain::GenerateNew(chunkIndex);     break;
    default:                    EN_ERROR("Unknown load mode!");
  }

  m_ChunkContainer.insert(std::move(chunk));
}

static Block::Type getBlockType(const Block::Type* blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k)
{
  EN_ASSERT(blockData, "Block data does not exist!");
  EN_ASSERT(-1 <= i && i <= Chunk::Size() && -1 <= j && j <= Chunk::Size() && -1 <= k && k <= Chunk::Size(), "Index is out of bounds!");
  return blockData[(Chunk::Size() + 2) * (Chunk::Size() + 2) * (i + 1) + (Chunk::Size() + 2) * (j + 1) + (k + 1)];
}

static Block::Type getBlockType(const Block::Type* blockData, const BlockIndex& blockIndex)
{
  return getBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k);
}

static void setBlockType(Block::Type* blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  EN_ASSERT(blockData, "Block data does not exist!");
  EN_ASSERT(-1 <= i && i <= Chunk::Size() && -1 <= j && j <= Chunk::Size() && -1 <= k && k <= Chunk::Size(), "Index is out of bounds!");
  blockData[(Chunk::Size() + 2) * (Chunk::Size() + 2) * (i + 1) + (Chunk::Size() + 2) * (j + 1) + (k + 1)] = blockType;
}

static void setBlockType(Block::Type* blockData, const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

std::vector<uint32_t> ChunkManager::createMesh(const GlobalIndex& chunkIndex) const
{
  static constexpr int totalVolumeNeeded = (Chunk::Size() + 2) * (Chunk::Size() + 2) * (Chunk::Size() + 2);
  thread_local Block::Type* const blockData = new Block::Type[totalVolumeNeeded];

  for (int i = 0; i < totalVolumeNeeded; ++i)
    blockData[i] = Block::Type::Null;

  // Load blocks from chunk
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    if (!chunk || chunk->empty())
      return {};

    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          setBlockType(blockData, i, j, k, chunk->getBlockType(i, j, k));
  }

  // Load blocks from cardinal neighbors
  for (Block::Face face : Block::FaceIterator())
  {
    int faceIndex = IsPositive(face) ? Chunk::Size() : -1;
    int neighborFaceIndex = IsPositive(face) ? 0 : Chunk::Size() - 1;

    auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(face));
    if (neighbor)
      for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
        for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
        {
          BlockIndex relativeBlockIndex = BlockIndex::CreatePermuted(faceIndex, i, j, GetCoordID(face));

          if (neighbor->empty())
            setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
          else
          {
            BlockIndex neighborBlockIndex = BlockIndex::CreatePermuted(neighborFaceIndex, i, j, GetCoordID(face));
            setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
          }
        }
  }

  // Load blocks from edge neighbors
  for (auto itA = Block::FaceIterator().begin(); itA != Block::FaceIterator().end(); ++itA)
    for (auto itB = itA.next(); itB != Block::FaceIterator().end(); ++itB)
    {
      Block::Face faceA = *itA;
      Block::Face faceB = *itB;

      // Opposite faces cannot form edge
      if (faceB == !faceA)
        continue;

      int u = GetCoordID(faceA);
      int v = GetCoordID(faceB);
      int w = (2 * (u + v)) % 3;  // Extracts coordID that runs along edge

      auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(faceA) + GlobalIndex::Dir(faceB));
      if (neighbor)
        for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
        {
          BlockIndex relativeBlockIndex{};
          relativeBlockIndex[u] = IsPositive(faceA) ? Chunk::Size() : -1;
          relativeBlockIndex[v] = IsPositive(faceB) ? Chunk::Size() : -1;
          relativeBlockIndex[w] = i;

          if (neighbor->empty())
            setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
          else
          {
            BlockIndex neighborBlockIndex{};
            neighborBlockIndex[u] = IsPositive(faceA) ? 0 : Chunk::Size() - 1;
            neighborBlockIndex[v] = IsPositive(faceB) ? 0 : Chunk::Size() - 1;
            neighborBlockIndex[w] = i;

            setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
          }
        }
    }

  // Load blocks from corner neighbors
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
      {
        GlobalIndex neighborIndex = chunkIndex + GlobalIndex(i, j, k) - GlobalIndex(1 - i, 1 - j, 1 - k);
        auto [neighbor, lock] = m_ChunkContainer.acquireChunk(neighborIndex);
        if (neighbor)
        {
          BlockIndex relativeBlockIndex = Chunk::Size() * BlockIndex(i, j, k) - BlockIndex(1 - i, 1 - j, 1 - k);

          if (neighbor->empty())
            setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
          else
          {
            BlockIndex neighborBlockIndex = static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex(1 - i, 1 - j, 1 - k);
            setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
          }
        }
      }

  static constexpr BlockIndex offsets[6][4]
    = { { {0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1} },    /*  West Face   */
        { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },    /*  East Face   */
        { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },    /*  South Face  */
        { {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1} },    /*  North Face  */
        { {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0} },    /*  Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} } };  /*  Top Face    */

  // Mesh chunk using block data
  std::vector<uint32_t> mesh;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
      {
        Block::Type blockType = getBlockType(blockData, i, j, k);

        if (blockType != Block::Type::Air)
          for (Block::Face face : Block::FaceIterator())
            if (Block::HasTransparency(getBlockType(blockData, BlockIndex(i, j, k) + BlockIndex::Dir(face))))
            {
              int textureID = static_cast<int>(Block::GetTexture(blockType, face));
              int faceID = static_cast<int>(face);
              int u = faceID / 2;
              int v = (u + 1) % 3;
              int w = (u + 2) % 3;

              for (int vert = 0; vert < 4; ++vert)
              {
                Block::Face sideADir = static_cast<Block::Face>(2 * v + offsets[faceID][vert][v]);
                Block::Face sideBDir = static_cast<Block::Face>(2 * w + offsets[faceID][vert][w]);

                BlockIndex sideA = BlockIndex(i, j, k) + BlockIndex::Dir(face) + BlockIndex::Dir(sideADir);
                BlockIndex sideB = BlockIndex(i, j, k) + BlockIndex::Dir(face) + BlockIndex::Dir(sideBDir);
                BlockIndex corner = BlockIndex(i, j, k) + BlockIndex::Dir(face) + BlockIndex::Dir(sideADir) + BlockIndex::Dir(sideBDir);

                bool sideAIsOpaque = !Block::HasTransparency(getBlockType(blockData, sideA));
                bool sideBIsOpaque = !Block::HasTransparency(getBlockType(blockData, sideB));
                bool cornerIsOpaque = !Block::HasTransparency(getBlockType(blockData, corner));
                int AO = sideAIsOpaque && sideBIsOpaque ? 0 : 3 - (sideAIsOpaque + sideBIsOpaque + cornerIsOpaque);

                BlockIndex vertexIndex = BlockIndex(i, j, k) + offsets[faceID][vert];

                uint32_t vertexData = vertexIndex.i + (vertexIndex.j << 6) + (vertexIndex.k << 12);   // Local vertex coordinates
                vertexData |= vert << 18;                                                             // Quad vertex index
                vertexData |= AO << 20;                                                               // Ambient occlusion value
                vertexData |= textureID << 22;                                                        // TextureID

                mesh.push_back(vertexData);
              }
            }
      }
  return mesh;
}