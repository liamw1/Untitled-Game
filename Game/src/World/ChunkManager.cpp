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
  if (!s_OpaqueVoxelShader)
  {
    s_OpaqueVoxelShader = Engine::Shader::Create("assets/shaders/Voxel.glsl", { { "TRANSPARENT", "0"} });
    s_TransparentVoxelShader = Engine::Shader::Create("assets/shaders/Voxel.glsl", { { "TRANSPARENT", "1"} });
    s_TextureArray = Block::GetTextureArray();

    s_SSBO = Engine::StorageBuffer::Create(Engine::StorageBuffer::Type::SSBO, c_StorageBufferBinding);
    s_SSBO->set(nullptr, c_StorageBufferSize);
    s_SSBO->bind();
  }

  m_OpaqueMultiDrawArray = std::make_unique<Engine::MultiDrawArray<Chunk::DrawCommand>>(s_VertexBufferLayout);
  m_TransparentMultiDrawArray = std::make_unique<Engine::MultiDrawArray<Chunk::DrawCommand>>(s_VertexBufferLayout);
}

void ChunkManager::render()
{
  EN_PROFILE_FUNCTION();

  const Mat4& viewProjection = Engine::Scene::CalculateViewProjection(Engine::Scene::ActiveCamera());
  std::array<Vec4, 6> frustumPlanes = Util::CalculateViewFrustumPlanes(viewProjection);

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
  static constexpr length_t chunkSphereRadius = Constants::SQRT3 * Chunk::Length() / 2;
  for (Vec4& plane : frustumPlanes)
  {
    length_t planeNormalMag = glm::length(Vec3(plane));
    plane.w += chunkSphereRadius * planeNormalMag;
  }

  Vec3 playerCameraPosition = Player::CameraPosition();
  GlobalIndex originIndex = Player::OriginIndex();

  s_TextureArray->bind(c_TextureSlot);

  {
    Engine::RenderCommand::SetDepthWriting(true);
    Engine::RenderCommand::SetUseDepthOffset(false);

    int commandCount = m_OpaqueMultiDrawArray->mask([&originIndex, &frustumPlanes](const GlobalIndex& chunkIndex)
      {
        Vec3 anchorPosition = Chunk::AnchorPosition(chunkIndex, originIndex);
        Vec3 chunkCenter = Chunk::Center(anchorPosition);
        return Util::IsInRange(chunkIndex, originIndex, c_RenderDistance) && Util::IsInFrustum(chunkCenter, frustumPlanes);
      });

    std::vector<Float4> storageBufferData;
    storageBufferData.reserve(commandCount);
    const std::vector<Chunk::DrawCommand>& drawCommands = m_OpaqueMultiDrawArray->getDrawCommandBuffer();
    for (int i = 0; i < commandCount; ++i)
    {
      const GlobalIndex& chunkIndex = drawCommands[i].id();
      Vec3 chunkAnchor = Chunk::AnchorPosition(chunkIndex, originIndex);
      storageBufferData.emplace_back(chunkAnchor, 0);
    }

    uint32_t bufferDataSize = static_cast<uint32_t>(storageBufferData.size() * sizeof(Float4));
    if (bufferDataSize > c_StorageBufferSize)
      EN_ERROR("Chunk anchor data exceeds SSBO size!");

    s_OpaqueVoxelShader->bind();
    m_OpaqueMultiDrawArray->bind();
    s_SSBO->update(storageBufferData.data(), 0, bufferDataSize);
    Engine::RenderCommand::MultiDrawVertices(drawCommands.data(), commandCount, sizeof(Chunk::DrawCommand));
  }

  {
    Engine::RenderCommand::SetBlending(true);
    Engine::RenderCommand::SetFaceCulling(false);
    Engine::RenderCommand::SetDepthWriting(false);
    Engine::RenderCommand::SetUseDepthOffset(true);
    Engine::RenderCommand::SetDepthOffset(-1.0f, -1.0f);

    int commandCount = m_TransparentMultiDrawArray->mask([&originIndex, &frustumPlanes](const GlobalIndex& chunkIndex)
      {
        Vec3 anchorPosition = Chunk::AnchorPosition(chunkIndex, originIndex);
        Vec3 chunkCenter = Chunk::Center(anchorPosition);
        return Util::IsInRange(chunkIndex, originIndex, c_RenderDistance) && Util::IsInFrustum(chunkCenter, frustumPlanes);
      });
    m_TransparentMultiDrawArray->sort(commandCount, [&originIndex, &playerCameraPosition](const GlobalIndex& chunkA, const GlobalIndex& chunkB)
      {
        // NOTE: Maybe measure min distance to chunk faces instead

        Vec3 anchorA = Chunk::AnchorPosition(chunkA, originIndex);
        Vec3 centerA = Chunk::Center(anchorA);
        length_t distA = glm::length2(centerA - playerCameraPosition);

        Vec3 anchorB = Chunk::AnchorPosition(chunkB, originIndex);
        Vec3 centerB = Chunk::Center(anchorB);
        length_t distB = glm::length2(centerB - playerCameraPosition);

        return distA > distB;
      });
    m_TransparentMultiDrawArray->amend(commandCount, [&originIndex, &playerCameraPosition](Chunk::DrawCommand& drawCommand)
      {
        drawCommand.sortVoxels(originIndex, playerCameraPosition);
        return true;
      });

    std::vector<Float4> storageBufferData;
    storageBufferData.reserve(commandCount);
    const std::vector<Chunk::DrawCommand>& drawCommands = m_TransparentMultiDrawArray->getDrawCommandBuffer();
    for (int i = 0; i < commandCount; ++i)
    {
      const GlobalIndex& chunkIndex = drawCommands[i].id();
      Vec3 chunkAnchor = Chunk::AnchorPosition(chunkIndex, originIndex);
      storageBufferData.emplace_back(chunkAnchor, 0);
    }

    uint32_t bufferDataSize = static_cast<uint32_t>(storageBufferData.size() * sizeof(Float4));
    if (bufferDataSize > c_StorageBufferSize)
      EN_ERROR("Chunk anchor data exceeds SSBO size!");

    s_TransparentVoxelShader->bind();
    m_TransparentMultiDrawArray->bind();
    s_SSBO->update(storageBufferData.data(), 0, bufferDataSize);
    Engine::RenderCommand::MultiDrawVertices(drawCommands.data(), commandCount, sizeof(Chunk::DrawCommand));
  }
}

void ChunkManager::update()
{
  EN_PROFILE_FUNCTION();

  std::optional<GlobalIndex> updateIndex = m_ChunkContainer.getForceUpdateIndex();
  while (updateIndex)
  {
    if (!m_ChunkContainer.contains(*updateIndex))
      generateNewChunk(*updateIndex);

    bool meshGenerated = meshChunk(*updateIndex);
    m_ChunkContainer.update(*updateIndex, meshGenerated);

    updateIndex = m_ChunkContainer.getForceUpdateIndex();
  }

  if (!m_OpaqueCommandQueue.empty())
    m_ChunkContainer.uploadMeshes(m_OpaqueCommandQueue, m_OpaqueMultiDrawArray);
  if (!m_TransparentCommandQueue.empty())
    m_ChunkContainer.uploadMeshes(m_TransparentCommandQueue, m_TransparentMultiDrawArray);
}

void ChunkManager::clean()
{
  GlobalIndex originIndex = Player::OriginIndex();
  if (m_PrevPlayerOriginIndex == originIndex)
    return;

  EN_PROFILE_FUNCTION();

  std::vector<GlobalIndex> chunksMarkedForDeletion = m_ChunkContainer.findAll(ChunkType::Boundary, [&originIndex](const Chunk& chunk)
    {
      return !Util::IsInRange(chunk.globalIndex(), originIndex, c_UnloadDistance);
    });

  if (!chunksMarkedForDeletion.empty())
  {
    for (const GlobalIndex& chunkIndex : chunksMarkedForDeletion)
      if (!Util::IsInRange(chunkIndex, originIndex, c_UnloadDistance))
      {
        m_ChunkContainer.erase(chunkIndex);
        m_OpaqueMultiDrawArray->remove(chunkIndex);
        m_TransparentMultiDrawArray->remove(chunkIndex);
      }
  }

  m_PrevPlayerOriginIndex = originIndex;
}

std::pair<const Chunk*, std::unique_lock<std::mutex>> ChunkManager::acquireChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return m_ChunkContainer.acquireChunk(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void ChunkManager::placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, Direction face, Block::Type blockType)
{
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);

    if (!chunk || !Block::HasCollision(chunk->getBlockType(blockIndex)))
    {
      EN_WARN("Cannot call placeBlock on block with no collision!");
      return;
    }

    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, face))
    {
      chunkIndex += GlobalIndex::Dir(face);
      auto [neighbor, neighborLock] = m_ChunkContainer.acquireChunk(chunkIndex);

      if (!neighbor)
        return;

      chunk = neighbor;
      lock = std::move(neighborLock);

      blockIndex -= static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::Dir(face);
    }
    else
      blockIndex += BlockIndex::Dir(face);

    // If trying to place an air block or trying to place a block in a space occupied by another block with collision, do nothing
    if (blockType == Block::Type::Air || (!chunk->empty() && Block::HasCollision(chunk->getBlockType(blockIndex))))
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

void ChunkManager::setLoadModeTerrain()
{
  m_LoadMode = LoadMode::Terrain;
}

void ChunkManager::setLoadModeVoid()
{
  m_LoadMode = LoadMode::Void;
}

void ChunkManager::launchLoadThread()
{
  m_LoadThread = std::thread(&ChunkManager::loadWorker, this);
}

void ChunkManager::launchUpdateThread()
{
  m_UpdateThread = std::thread(&ChunkManager::updateWorker, this);
}

void ChunkManager::loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType)
{
  Chunk chunk(chunkIndex);
  if (blockType != Block::Type::Air)
  {
    chunk.setComposition(AllocateArray3D<Block::Type, Chunk::Size()>(blockType));
    chunk.setLighting(AllocateArray3D<Block::Light, Chunk::Size()>());
  }

  m_ChunkContainer.insert(std::move(chunk));
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
      bool meshGenerated = meshChunk(*updateIndex);
      m_ChunkContainer.update(*updateIndex, meshGenerated);
    }
    else
      std::this_thread::sleep_for(100ms);
  }
}

void ChunkManager::generateNewChunk(const GlobalIndex& chunkIndex)
{
  Chunk chunk;
  switch (m_LoadMode)
  {
    case LoadMode::NotSet:  EN_ERROR("Load mode not set!");                     break;
    case LoadMode::Void:    chunk = Terrain::GenerateEmpty(chunkIndex);   break;
    case LoadMode::Terrain: chunk = Terrain::GenerateNew(chunkIndex);     break;
    default:                EN_ERROR("Unknown load mode!");
  }

  m_ChunkContainer.insert(std::move(chunk));
}



ChunkManager::BlockData::BlockData()
  : m_Type(AllocateArray3D<Block::Type, c_Size>(Block::Type::Null)),
    m_Light(AllocateArray3D<Block::Light, c_Size>()) {}

Block::Type ChunkManager::BlockData::getType(const BlockIndex& blockIndex) const
{
  return m_Type[blockIndex.i + 1][blockIndex.j + 1][blockIndex.k + 1];
}

Block::Light ChunkManager::BlockData::getLight(const BlockIndex& blockIndex) const
{
  return m_Light[blockIndex.i + 1][blockIndex.j + 1][blockIndex.k + 1];
}

void ChunkManager::BlockData::set(const BlockIndex& dataIndex, Block::Type blockType, Block::Light blockLight)
{
  m_Type[dataIndex.i + 1][dataIndex.j + 1][dataIndex.k + 1] = blockType;
  m_Light[dataIndex.i + 1][dataIndex.j + 1][dataIndex.k + 1] = blockLight;
}

void ChunkManager::BlockData::set(const BlockIndex& dataIndex, const Chunk* chunk, const BlockIndex& blockIndex)
{
  set(dataIndex, chunk->getBlockType(blockIndex), chunk->getBlockLight(blockIndex));
}

void ChunkManager::BlockData::reset()
{
  m_Type.fill(Block::Type::Null);
  m_Light.fill(Block::Light());
}

bool ChunkManager::BlockData::empty() const
{
  return m_Type[1][1][1] == Block::Type::Null;
}



const ChunkManager::BlockData& ChunkManager::getBlockData(const GlobalIndex& chunkIndex) const
{
  thread_local BlockData blockData{};
  blockData.reset();

  // Load blocks from chunk
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    if (!chunk || chunk->empty())
      return blockData;

    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
        {
          BlockIndex blockIndex(i, j, k);
          blockData.set(blockIndex, chunk, blockIndex);
        }
  }

  EN_PROFILE_FUNCTION();

  static constexpr blockIndex_t neighborLimits[2] = { Chunk::Size() - 1, 0 };
  static constexpr blockIndex_t neighborRelativeIndices[2] = { -1, Chunk::Size() };

  // Load blocks from cardinal neighbors
  for (Direction direction : Directions())
  {
    auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(direction));
    if (neighbor)
    {
      blockIndex_t neighborFaceIndex = neighborLimits[IsUpstream(direction)];
      blockIndex_t faceRelativeIndex = neighborRelativeIndices[IsUpstream(direction)];

      for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
        for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
        {
          BlockIndex relativeBlockIndex = BlockIndex::CreatePermuted(faceRelativeIndex, i, j, GetCoordID(direction));

          if (!neighbor->empty())
          {
            BlockIndex neighborBlockIndex = BlockIndex::CreatePermuted(neighborFaceIndex, i, j, GetCoordID(direction));
            blockData.set(relativeBlockIndex, neighbor, neighborBlockIndex);
          }
          else
            blockData.set(relativeBlockIndex, Block::Type::Air, Block::Light(Block::Light::MaxValue()));
        }
    }
  }

  // Load blocks from edge neighbors
  for (auto itA = Directions().begin(); itA != Directions().end(); ++itA)
    for (auto itB = itA.next(); itB != Directions().end(); ++itB)
    {
      Direction faceA = *itA;
      Direction faceB = *itB;

      // Opposite faces cannot form edge
      if (faceB == !faceA)
        continue;

      auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(faceA) + GlobalIndex::Dir(faceB));
      if (neighbor)
      {
        int u = GetCoordID(faceA);
        int v = GetCoordID(faceB);
        int w = (2 * (u + v)) % 3;  // Extracts coordID that runs along edge

        for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
        {
          BlockIndex relativeBlockIndex;
          relativeBlockIndex[u] = neighborRelativeIndices[IsUpstream(faceA)];
          relativeBlockIndex[v] = neighborRelativeIndices[IsUpstream(faceB)];
          relativeBlockIndex[w] = i;

          if (!neighbor->empty())
          {
            BlockIndex neighborBlockIndex;
            neighborBlockIndex[u] = neighborLimits[IsUpstream(faceA)];
            neighborBlockIndex[v] = neighborLimits[IsUpstream(faceB)];
            neighborBlockIndex[w] = i;

            blockData.set(relativeBlockIndex, neighbor, neighborBlockIndex);
          }
          else
            blockData.set(relativeBlockIndex, Block::Type::Air, Block::Light(Block::Light::MaxValue()));
        }
      }
    }

  // Load blocks from corner neighbors
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
      {
        GlobalIndex neighborIndex = chunkIndex + 2 * GlobalIndex(i, j, k) - 1;

        auto [neighbor, lock] = m_ChunkContainer.acquireChunk(neighborIndex);
        if (neighbor)
        {
          BlockIndex relativeBlockIndex = { neighborRelativeIndices[i], neighborRelativeIndices[j], neighborRelativeIndices[k] };

          if (!neighbor->empty())
          {
            BlockIndex neighborBlockIndex = { neighborLimits[i], neighborLimits[j], neighborLimits[k] };
            blockData.set(relativeBlockIndex, neighbor, neighborBlockIndex);
          }
          else
            blockData.set(relativeBlockIndex, Block::Type::Air, Block::Light(Block::Light::MaxValue()));
        }
      }

  return blockData;
}

bool ChunkManager::meshChunk(const GlobalIndex& chunkIndex)
{
  const BlockData& blockData = getBlockData(chunkIndex);

  if (blockData.empty())
  {
    m_OpaqueCommandQueue.emplace(chunkIndex);
    m_TransparentCommandQueue.emplace(chunkIndex);
    return false;
  }

  EN_PROFILE_FUNCTION();

  static constexpr BlockIndex offsets[6][4]
    = { { {0, 1, 0}, {0, 0, 0}, {0, 1, 1}, {0, 0, 1} },    /*  West Face   */
        { {1, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 1, 1} },    /*  East Face   */
        { {0, 0, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0, 1} },    /*  South Face  */
        { {1, 1, 0}, {0, 1, 0}, {1, 1, 1}, {0, 1, 1} },    /*  North Face  */
        { {0, 1, 0}, {1, 1, 0}, {0, 0, 0}, {1, 0, 0} },    /*  Bottom Face */
        { {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1} } };  /*  Top Face    */

  std::vector<Chunk::Voxel> opaqueMesh;
  std::vector<Chunk::Voxel> transparentMesh;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
      {
        BlockIndex blockIndex = BlockIndex(i, j, k);
        Block::Type blockType = blockData.getType(blockIndex);

        // Air blocks do not get meshed
        if (blockType == Block::Type::Air)
          continue;

        uint32_t quadData1 = 0;
        uint32_t quadData2 = 0;
        for (Direction direction : Directions())
        {
          // Only enable face if block and neighbor are different types, or if either are transparent
          Block::Type cardinalNeighbor = blockData.getType(blockIndex + BlockIndex::Dir(direction));
          if (cardinalNeighbor == blockType || (!Block::HasTransparency(blockType) && !Block::HasTransparency(cardinalNeighbor)))
            continue;

          int directionID = static_cast<int>(direction);
          quadData2 |= 1 << (16 + directionID);

          // If quad texture is transparent, no need to set ambient occlusion
          if (Block::HasTransparency(Block::GetTexture(blockType, direction)))
            continue;

          for (int quadIndex = 0; quadIndex < 4; ++quadIndex)
          {
            int u = directionID / 2;
            int v = (u + 1) % 3;
            int w = (u + 2) % 3;

            Direction edgeADir = static_cast<Direction>(2 * v + offsets[directionID][quadIndex][v]);
            Direction edgeBDir = static_cast<Direction>(2 * w + offsets[directionID][quadIndex][w]);

            BlockIndex edgeA = blockIndex + BlockIndex::Dir(direction) + BlockIndex::Dir(edgeADir);
            BlockIndex edgeB = blockIndex + BlockIndex::Dir(direction) + BlockIndex::Dir(edgeBDir);
            BlockIndex corner = blockIndex + BlockIndex::Dir(direction) + BlockIndex::Dir(edgeADir) + BlockIndex::Dir(edgeBDir);

            bool sideAIsOpaque = !Block::HasTransparency(blockData.getType(edgeA));
            bool sideBIsOpaque = !Block::HasTransparency(blockData.getType(edgeB));
            bool cornerIsOpaque = !Block::HasTransparency(blockData.getType(corner));
            int ambientOcclusionLevel = sideAIsOpaque && sideBIsOpaque ? 3 : sideAIsOpaque + sideBIsOpaque + cornerIsOpaque;

            int shift = 2 * (4 * directionID + quadIndex);
            if (shift < 32)
              quadData1 |= ambientOcclusionLevel << shift;
            else
              quadData2 |= ambientOcclusionLevel << (shift - 32);
          }
        }

        // Skip block if no faces are enabled
        if (!quadData2)
          continue;

        uint32_t voxelData = static_cast<blockID>(blockType);
        voxelData |= blockIndex.i << 16;
        voxelData |= blockIndex.j << 21;
        voxelData |= blockIndex.k << 26;

        Array3D lights = AllocateArray3D<std::optional<int8_t>, 3>(std::nullopt);
        for (blockIndex_t I = 0; I < 3; ++I)
          for (blockIndex_t J = 0; J < 3; ++J)
            for (blockIndex_t K = 0; K < 3; ++K)
            {
              BlockIndex index = blockIndex + BlockIndex(I - 1, J - 1, K - 1);
              if (Block::HasTransparency(blockData.getType(index)))
                lights[I][J][K] = blockData.getLight(index).sunlight();
            }

        uint32_t sunlight = 0;
        for (blockIndex_t vI = 0; vI < 2; ++vI)
          for (blockIndex_t vJ = 0; vJ < 2; ++vJ)
            for (blockIndex_t vK = 0; vK < 2; ++vK)
            {
              int count = 0;
              int8_t lightVal = 0;
              for (blockIndex_t I = 0; I < 2; ++I)
                for (blockIndex_t J = 0; J < 2; ++J)
                  for (blockIndex_t K = 0; K < 2; ++K)
                    if (lights[I + vI][J + vJ][K + vK])
                    {
                      lightVal += *lights[I + vI][J + vJ][K + vK];
                      count++;
                    }
              lightVal /= std::max(1, count);

              int vertexIndex = 4 * vI + 2 * vJ + vK;
              int shift = 4 * vertexIndex;
              sunlight |= lightVal << shift;
            }

        std::vector<Chunk::Voxel>& mesh = Block::HasTransparency(blockType) ? transparentMesh : opaqueMesh;
        mesh.push_back(Chunk::Voxel(voxelData, quadData1, quadData2, sunlight));
      }

  bool meshGenrated = !opaqueMesh.empty() || !transparentMesh.empty();
  m_OpaqueCommandQueue.emplace(chunkIndex, std::move(opaqueMesh));
  m_TransparentCommandQueue.emplace(chunkIndex, std::move(transparentMesh));
  return meshGenrated;
}