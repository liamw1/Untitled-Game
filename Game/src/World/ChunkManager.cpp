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
  if (!s_Shader)
  {
    s_Shader = Engine::Shader::Create("assets/shaders/Chunk.glsl");
    s_TextureArray = Block::GetTextureArray();

    s_SSBO = Engine::StorageBuffer::Create(Engine::StorageBuffer::Type::SSBO, c_StorageBufferBinding);
    s_SSBO->set(nullptr, c_StorageBufferSize);
    s_SSBO->bind();
  }

  m_OpaqueMultiDrawArray = std::make_unique<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>(s_VertexBufferLayout);
  m_TransparentMultiDrawArray = std::make_unique<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>(s_VertexBufferLayout);
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

  s_Shader->bind();
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

    m_OpaqueMultiDrawArray->bind();
    s_SSBO->update(storageBufferData.data(), 0, bufferDataSize);
    Engine::RenderCommand::MultiDrawIndexed(drawCommands.data(), commandCount, sizeof(Chunk::DrawCommand));
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
        bool orderModified = drawCommand.sort(originIndex, playerCameraPosition);
        return orderModified;
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

    m_TransparentMultiDrawArray->bind();
    s_SSBO->update(storageBufferData.data(), 0, bufferDataSize);
    Engine::RenderCommand::MultiDrawIndexed(drawCommands.data(), commandCount, sizeof(Chunk::DrawCommand));
  }
}

void ChunkManager::update()
{
  EN_PROFILE_FUNCTION();

  std::optional<GlobalIndex> updateIndex = m_ForceMeshUpdateQueue.tryRemove();
  while (updateIndex)
  {
    if (!m_ChunkContainer.contains(*updateIndex))
      generateNewChunk(*updateIndex);

    meshChunk(*updateIndex);
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(*updateIndex);
    if (chunk)
      chunk->update();

    updateIndex = m_ForceMeshUpdateQueue.tryRemove();
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

  std::vector<GlobalIndex> chunksMarkedForDeletion = m_ChunkContainer.findAll([&originIndex](const Chunk& chunk)
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

const ConstChunkWithLock ChunkManager::acquireChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return m_ChunkContainer.acquireChunk(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void ChunkManager::placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, Direction face, Block::Type blockType)
{
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);

    if (!chunk || !chunk->composition() || !Block::HasCollision(chunk->composition()(blockIndex)))
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
    if (blockType == Block::Type::Air || (chunk->composition() && Block::HasCollision(chunk->composition()(blockIndex))))
    {
      EN_WARN("Invalid block placement!");
      return;
    }

    chunk->setBlockType(blockIndex, blockType);
  }

  sendBlockUpdate(chunkIndex, blockIndex);
}

void ChunkManager::removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    chunk->setBlockType(blockIndex, Block::Type::Air);
  }

  sendBlockUpdate(chunkIndex, blockIndex);
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
    chunk.setComposition(MakeCubicArray<Block::Type, Chunk::Size()>(blockType));
    chunk.setLighting(MakeCubicArray<Block::Light, Chunk::Size()>());
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

    std::optional<GlobalIndex> updateIndex = m_LazyMeshUpdateQueue.tryRemove();
    if (!updateIndex)
    {
      std::this_thread::sleep_for(100ms);
      continue;
    }
    if (m_ChunkContainer.hasBoundaryNeighbors(*updateIndex))
      continue;

    meshChunk(*updateIndex);
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(*updateIndex);
    if (chunk)
      chunk->update();
  }
}

void ChunkManager::addToLazyUpdateQueue(const GlobalIndex& chunkIndex)
{
  if (!m_ForceMeshUpdateQueue.contains(chunkIndex))
    m_LazyMeshUpdateQueue.add(chunkIndex);
}

void ChunkManager::addToForceUpdateQueue(const GlobalIndex& chunkIndex)
{
  m_LazyMeshUpdateQueue.remove(chunkIndex);
  m_ForceMeshUpdateQueue.add(chunkIndex);
}

void ChunkManager::generateNewChunk(const GlobalIndex& chunkIndex)
{
  EN_PROFILE_FUNCTION();

  Chunk chunk;
  switch (m_LoadMode)
  {
    case LoadMode::NotSet:  EN_ERROR("Load mode not set!");               break;
    case LoadMode::Void:    chunk = Terrain::GenerateEmpty(chunkIndex);   break;
    case LoadMode::Terrain: chunk = Terrain::GenerateNew(chunkIndex);     break;
    default:                EN_ERROR("Unknown load mode!");
  }

  bool insertionSuccess = m_ChunkContainer.insert(std::move(chunk));
  if (insertionSuccess)
    for (int i = -1; i <= 1; ++i)
      for (int j = -1; j <= 1; ++j)
        for (int k = -1; k <= 1; ++k)
        {
          GlobalIndex neighborIndex = chunkIndex + GlobalIndex(i, j, k);
          m_LazyMeshUpdateQueue.add(neighborIndex);
        }
}

void ChunkManager::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  m_ForceMeshUpdateQueue.add(chunkIndex);

  std::vector<Direction> updateDirections{};
  for (Direction direction : Directions())
    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, direction))
      updateDirections.push_back(direction);

  EN_ASSERT(updateDirections.size() <= 3, "Too many update directions for a single block update!");

  // Update neighbors in cardinal directions
  for (Direction direction : updateDirections)
  {
    GlobalIndex neighborIndex = chunkIndex + GlobalIndex::Dir(direction);
    m_ForceMeshUpdateQueue.add(neighborIndex);
  }

  // Queue edge neighbor for updating
  if (updateDirections.size() == 2)
  {
    GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::Dir(updateDirections[0]) + GlobalIndex::Dir(updateDirections[1]);
    m_LazyMeshUpdateQueue.add(edgeNeighborIndex);
  }

  // Queue edge/corner neighbors for updating
  if (updateDirections.size() == 3)
  {
    GlobalIndex cornerNeighborIndex = chunkIndex;
    for (int i = 0; i < 3; ++i)
    {
      int j = (i + 1) % 3;
      GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::Dir(updateDirections[i]) + GlobalIndex::Dir(updateDirections[j]);
      m_LazyMeshUpdateQueue.add(edgeNeighborIndex);

      cornerNeighborIndex += GlobalIndex::Dir(updateDirections[i]);
    }

    m_LazyMeshUpdateQueue.add(cornerNeighborIndex);
  }
}



ChunkManager::BlockData::BlockData()
  : composition(MakeCubicArray<Block::Type, Chunk::Size() + 2, -1>()),
    lighting(MakeCubicArray<Block::Light, Chunk::Size() + 2, -1>()) {}

void ChunkManager::BlockData::fill(const BlockBox& fillSection, const Chunk* chunk, const BlockIndex& chunkBase)
{
  if (chunk->composition())
    composition.fill(fillSection, chunk->composition(), chunkBase);
  else
    composition.fill(fillSection, Block::Type::Air);

  if (chunk->lighting())
    lighting.fill(fillSection, chunk->lighting(), chunkBase);
  else
    lighting.fill(fillSection, Block::Light::MaxValue());
}

const ChunkManager::BlockData& ChunkManager::getBlockData(const GlobalIndex& chunkIndex) const
{
  thread_local BlockData blockData;
  blockData.composition.fill(Block::Type::Null);
  blockData.lighting.fill(Block::Light());

  // Load blocks from chunk
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    if (!chunk || !chunk->composition())
      return blockData;

    BlockBox fillSection(0, Chunk::Size());
    blockData.fill(fillSection, chunk, BlockIndex(0));
  }

  static constexpr blockIndex_t neighborLimits[2] = { Chunk::Size() - 1, 0 };
  static constexpr blockIndex_t neighborRelativeIndices[2] = { -1, Chunk::Size() };

  // Load blocks from cardinal neighbors
  for (Direction direction : Directions())
  {
    blockIndex_t neighborFaceIndex = neighborLimits[IsUpstream(direction)];
    blockIndex_t faceRelativeIndex = neighborRelativeIndices[IsUpstream(direction)];

    BlockIndex fillLower = BlockIndex::CreatePermuted(faceRelativeIndex, 0, 0, GetCoordID(direction));
    BlockIndex fillUpper = BlockIndex::CreatePermuted(faceRelativeIndex + 1, Chunk::Size(), Chunk::Size(), GetCoordID(direction));
    BlockBox fillSection(fillLower, fillUpper);

    auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(direction));
    if (!neighbor)
      continue;

    BlockIndex neighborBase = BlockIndex::CreatePermuted(neighborFaceIndex, 0, 0, GetCoordID(direction));
    blockData.fill(fillSection, neighbor, neighborBase);
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
      if (!neighbor)
        continue;

      int u = GetCoordID(faceA);
      int v = GetCoordID(faceB);
      int w = (2 * (u + v)) % 3;  // Extracts coordID that runs along edge

      BlockIndex fillLower(0);
      fillLower[u] = neighborRelativeIndices[IsUpstream(faceA)];
      fillLower[v] = neighborRelativeIndices[IsUpstream(faceB)];
      BlockIndex fillUpper(Chunk::Size());
      fillUpper[u] = fillLower[u] + 1;
      fillUpper[v] = fillLower[v] + 1;
      BlockBox fillSection(fillLower, fillUpper);

      BlockIndex neighborBase(0);
      neighborBase[u] = neighborLimits[IsUpstream(faceA)];
      neighborBase[v] = neighborLimits[IsUpstream(faceB)];

      blockData.fill(fillSection, neighbor, neighborBase);
    }

  // Load blocks from corner neighbors
  for (int i = -1; i < 2; i += 2)
    for (int j = -1; j < 2; j += 2)
      for (int k = -1; k < 2; k += 2)
      {
        GlobalIndex neighborIndex = chunkIndex + GlobalIndex(i, j, k);

        auto [neighbor, lock] = m_ChunkContainer.acquireChunk(neighborIndex);
        if (!neighbor)
          continue;

        BlockIndex fillIndex = { neighborRelativeIndices[i > 0], neighborRelativeIndices[j > 0], neighborRelativeIndices[k > 0] };
        BlockIndex neighborBlockIndex = { neighborLimits[i > 0], neighborLimits[j > 0], neighborLimits[k > 0] };

        blockData.fill({ fillIndex, fillIndex + 1 }, neighbor, neighborBlockIndex);
      }

  return blockData;
}

void ChunkManager::meshChunk(const GlobalIndex& chunkIndex)
{
  const BlockData& blockData = getBlockData(chunkIndex);

  if (blockData.composition(BlockIndex(0)) == Block::Type::Null)
  {
    m_OpaqueCommandQueue.emplace(chunkIndex, false);
    m_TransparentCommandQueue.emplace(chunkIndex, false);
    return;
  }

  EN_PROFILE_FUNCTION();

  Chunk::DrawCommand opaqueDraw(chunkIndex, true);
  Chunk::DrawCommand transparentDraw(chunkIndex, false);
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
      {
        BlockIndex blockIndex(i, j, k);
        Block::Type blockType = blockData.composition(blockIndex);

        if (blockType == Block::Type::Air)
          continue;

        uint8_t enabledFaces = 0;
        bool blockIsTransparent = Block::HasTransparency(blockType);
        Chunk::DrawCommand& draw = blockIsTransparent ? transparentDraw : opaqueDraw;
        for (Direction face : Directions())
        {
          BlockIndex cardinalIndex = blockIndex + BlockIndex::Dir(face);
          Block::Type cardinalNeighbor = blockData.composition(cardinalIndex);
          if (cardinalNeighbor == blockType || (!blockIsTransparent && !Block::HasTransparency(cardinalNeighbor)))
            continue;

          enabledFaces |= 1 << static_cast<int>(face);

          // Calculate lighting
          std::array<int, 4> sunlight{};
          for (int quadIndex = 0; quadIndex < 4; ++quadIndex)
          {
            int transparentNeighbors = 0;
            int totalSunlight = 0;
            BlockIndex vertexPosition = blockIndex + Chunk::Vertex::GetOffset(face, quadIndex);
            for (int I = -1; I <= 0; ++I)
              for (int J = -1; J <= 0; ++J)
                for (int K = -1; K <= 0; ++K)
                {
                  BlockIndex neighbor = vertexPosition + BlockIndex(I, J, K);
                  if (Block::HasTransparency(blockData.composition(neighbor)))
                  {
                    totalSunlight += blockData.lighting(neighbor).sunlight();
                    transparentNeighbors++;
                  }
                }

            sunlight[quadIndex] = totalSunlight / std::max(transparentNeighbors, 1);
          }

          // Calculate ambient occlusion
          std::array<int, 4> quadAmbientOcclusion{};
          Block::Texture quadTexture = Block::GetTexture(blockType, face);
          if (!Block::HasTransparency(quadTexture))
            for (int quadIndex = 0; quadIndex < 4; ++quadIndex)
            {
              int u = GetCoordID(face);
              int v = (u + 1) % 3;
              int w = (u + 2) % 3;

              Direction edgeADir = FromCoordID(v, Chunk::Vertex::GetOffset(face, quadIndex)[v]);
              Direction edgeBDir = FromCoordID(w, Chunk::Vertex::GetOffset(face, quadIndex)[w]);

              BlockIndex edgeA = blockIndex + BlockIndex::Dir(face) + BlockIndex::Dir(edgeADir);
              BlockIndex edgeB = blockIndex + BlockIndex::Dir(face) + BlockIndex::Dir(edgeBDir);
              BlockIndex corner = blockIndex + BlockIndex::Dir(face) + BlockIndex::Dir(edgeADir) + BlockIndex::Dir(edgeBDir);

              bool edgeAIsOpaque = !Block::HasTransparency(blockData.composition(edgeA));
              bool edgeBIsOpaque = !Block::HasTransparency(blockData.composition(edgeB));
              bool cornerIsOpaque = !Block::HasTransparency(blockData.composition(corner));
              quadAmbientOcclusion[quadIndex] = edgeAIsOpaque && edgeBIsOpaque ? 3 : edgeAIsOpaque + edgeBIsOpaque + cornerIsOpaque;
            }

          draw.addQuad(blockIndex, face, quadTexture, sunlight, quadAmbientOcclusion);
        }

        if (enabledFaces == 0)
          continue;

        draw.addVoxel(blockIndex, enabledFaces);
      }
  opaqueDraw.setIndices();

  m_OpaqueCommandQueue.add(std::move(opaqueDraw));
  m_TransparentCommandQueue.add(std::move(transparentDraw));
}