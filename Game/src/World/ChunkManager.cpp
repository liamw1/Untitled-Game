#include "GMpch.h"
#include "ChunkManager.h"
#include "Player/Player.h"
#include "Terrain.h"
#include "Util/Util.h"

ChunkManager::ChunkManager()
  : m_ThreadPool(std::make_shared<Engine::Threads::ThreadPool>(1)),
    m_LoadWork(m_ThreadPool, Engine::Threads::Priority::Normal),
    m_LightingWork(m_ThreadPool, Engine::Threads::Priority::Normal),
    m_LazyMeshingWork(m_ThreadPool, Engine::Threads::Priority::Low),
    m_ForceMeshingWork(m_ThreadPool, Engine::Threads::Priority::High),
    m_PrevPlayerOriginIndex(Player::OriginIndex()) {}

ChunkManager::~ChunkManager()
{
  m_ThreadPool->shutdown();
}

void ChunkManager::initialize()
{
  if (!s_Shader)
  {
    s_Shader = Engine::Shader::Create("assets/shaders/Chunk.glsl");
    s_LightUniform = Engine::Uniform::Create(2, sizeof(LightUniforms));
    s_TextureArray = Block::GetTextureArray();

    s_SSBO = Engine::StorageBuffer::Create(Engine::StorageBuffer::Type::SSBO, c_StorageBufferBinding);
    s_SSBO->set(nullptr, c_StorageBufferSize);
    s_SSBO->bind();
  }

  m_OpaqueMultiDrawArray = std::make_unique<Engine::MultiDrawIndexedArray<ChunkDrawCommand>>(s_VertexBufferLayout);
  m_TransparentMultiDrawArray = std::make_unique<Engine::MultiDrawIndexedArray<ChunkDrawCommand>>(s_VertexBufferLayout);

  LightUniforms lightUniforms;
  lightUniforms.sunIntensity = 1.0f;
  s_LightUniform->set(&lightUniforms, sizeof(LightUniforms));
}

void ChunkManager::render()
{
  EN_PROFILE_FUNCTION();

  const Mat4& viewProjection = Engine::Scene::CalculateViewProjection(Engine::Scene::ActiveCamera());
  std::array<Vec4, 6> frustumPlanes = Util::CalculateViewFrustumPlanes(viewProjection);

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
  static constexpr length_t chunkSphereRadius = std::numbers::sqrt3_v<length_t> * Chunk::Length() / 2;
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
    const std::vector<ChunkDrawCommand>& drawCommands = m_OpaqueMultiDrawArray->getDrawCommandBuffer();
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
    Engine::RenderCommand::MultiDrawIndexed(drawCommands.data(), commandCount, sizeof(ChunkDrawCommand));
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
    m_TransparentMultiDrawArray->amend(commandCount, [&originIndex, &playerCameraPosition](ChunkDrawCommand& drawCommand)
      {
        bool orderModified = drawCommand.sort(originIndex, playerCameraPosition);
        return orderModified;
      });

    std::vector<Float4> storageBufferData;
    storageBufferData.reserve(commandCount);
    const std::vector<ChunkDrawCommand>& drawCommands = m_TransparentMultiDrawArray->getDrawCommandBuffer();
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
    Engine::RenderCommand::MultiDrawIndexed(drawCommands.data(), commandCount, sizeof(ChunkDrawCommand));
  }
}

void ChunkManager::update()
{
  EN_PROFILE_FUNCTION();

  m_ForceMeshingWork.waitAndDiscardSaved();

  if (!m_OpaqueCommandQueue.empty())
    uploadMeshes(m_OpaqueCommandQueue, m_OpaqueMultiDrawArray);
  if (!m_TransparentCommandQueue.empty())
    uploadMeshes(m_TransparentCommandQueue, m_TransparentMultiDrawArray);
}

void ChunkManager::clean()
{
  GlobalIndex originIndex = Player::OriginIndex();
  if (m_PrevPlayerOriginIndex == originIndex)
    return;

  EN_PROFILE_FUNCTION();

  std::vector<GlobalIndex> chunksMarkedForDeletion = m_ChunkContainer.chunks().getKeys([&originIndex](const GlobalIndex& chunkIndex)
    {
      return !Util::IsInRange(chunkIndex, originIndex, c_UnloadDistance);
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

void ChunkManager::loadNewChunks()
{
  if (m_LoadWork.queuedTasks() > 0)
    return;

  if (m_LoadWork.savedResults() > 0)
    m_LoadWork.waitAndDiscardSaved();

  EN_PROFILE_FUNCTION();

  std::unordered_set<GlobalIndex> newChunkIndices = m_ChunkContainer.findAllLoadableIndices();

  // Load First chunk if none exist
  if (newChunkIndices.empty() && m_ChunkContainer.chunks().empty())
    newChunkIndices.insert(Player::OriginIndex());

  if (!newChunkIndices.empty())
    for (const GlobalIndex& newChunkIndex : newChunkIndices)
      m_LoadWork.submitAndSaveResult(newChunkIndex, &ChunkManager::generateNewChunk, this, newChunkIndex);
}

ChunkWithLock ChunkManager::acquireChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = Player::OriginIndex();
  return m_ChunkContainer.acquireChunk(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void ChunkManager::placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, Direction face, Block::Type blockType)
{
  if (Util::BlockNeighborIsInAnotherChunk(blockIndex, face))
  {
    chunkIndex += GlobalIndex::Dir(face);
    blockIndex -= static_cast<blockIndex_t>(Chunk::Size() - 1) * BlockIndex::Dir(face);
  }
  else
    blockIndex += BlockIndex::Dir(face);

  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    if (!chunk)
      return;

    // If trying to place an air block or trying to place a block in a space occupied by another block with collision, do nothing
    if (blockType == Block::Type::Air || Block::HasCollision(chunk->getBlockType(blockIndex)))
    {
      EN_WARN("Invalid block placement!");
      return;
    }

    chunk->setBlockType(blockIndex, blockType);
  }

  if (!Block::HasTransparency(blockType))
    addToLightingUpdateQueue(chunkIndex);
  sendBlockUpdate(chunkIndex, blockIndex);
}

void ChunkManager::removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  Block::Type removedBlock = Block::Type::Null;

  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);

    removedBlock = chunk->getBlockType(blockIndex);
    chunk->setBlockType(blockIndex, Block::Type::Air);

    // Update lighting for effected block for immediate meshing
    for (Direction direction : Directions())
    {
      BlockIndex blockNeighbor = blockIndex + BlockIndex::Dir(direction);
      if (!BlockBox(0, Chunk::Size()).encloses(blockNeighbor) || !Block::HasTransparency(chunk->getBlockType(blockNeighbor)))
        continue;

      chunk->setBlockLight(blockIndex, chunk->getBlockLight(blockNeighbor));
      break;
    }
  }

  if (!Block::HasTransparency(removedBlock))
    addToLightingUpdateQueue(chunkIndex);
  sendBlockUpdate(chunkIndex, blockIndex);
}

void ChunkManager::loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType)
{
  std::shared_ptr chunk = std::make_shared<Chunk>(chunkIndex);
  if (blockType != Block::Type::Air)
  {
    chunk->setComposition(ArrayBox<Block::Type, blockIndex_t, 0, Chunk::Size()>(blockType));
    chunk->setLighting(ArrayBox<Block::Light, blockIndex_t, 0, Chunk::Size()>(AllocationPolicy::ForOverWrite));
  }

  m_ChunkContainer.insert(chunkIndex, std::move(chunk));
}



ChunkManager::BlockData::BlockData()
  : composition(ArrayBox<Block::Type, blockIndex_t, -1, Chunk::Size() + 1>(AllocationPolicy::ForOverWrite)),
    lighting(ArrayBox<Block::Light, blockIndex_t, -1, Chunk::Size() + 1>(AllocationPolicy::ForOverWrite)) {}

void ChunkManager::BlockData::fill(const BlockBox& fillSection, std::shared_ptr<Chunk> chunk, const BlockBox& chunkSection, bool fillLighting)
{
  if (chunk->composition())
    composition.fill(fillSection, chunk->composition(), chunkSection);
  else
    composition.fill(fillSection, Block::Type::Air);

  if (!fillLighting)
    return;

  if (chunk->lighting())
    lighting.fill(fillSection, chunk->lighting(), chunkSection);
  else
    lighting.fill(fillSection, Block::Light::MaxValue());
}



void ChunkManager::addToLightingUpdateQueue(const GlobalIndex& chunkIndex)
{
  m_LightingWork.submit(chunkIndex, &ChunkManager::lightingPacket, this, chunkIndex);
}

void ChunkManager::addToLazyMeshUpdateQueue(const GlobalIndex& chunkIndex)
{
  if (m_ForceMeshingWork.contains(chunkIndex))
    return;

  m_LazyMeshingWork.submit(chunkIndex, &ChunkManager::lazyMeshingPacket, this, chunkIndex);
}

void ChunkManager::addToForceMeshUpdateQueue(const GlobalIndex& chunkIndex)
{
  m_ForceMeshingWork.submitAndSaveResult(chunkIndex, &ChunkManager::forceMeshingPacket, this, chunkIndex);
}

void ChunkManager::generateNewChunk(const GlobalIndex& chunkIndex)
{
  EN_PROFILE_FUNCTION();

  std::shared_ptr<Chunk> chunk = Terrain::GenerateNew(chunkIndex);

  bool insertionSuccess = m_ChunkContainer.insert(chunkIndex, std::move(chunk));
  if (insertionSuccess)
    Chunk::Stencil(chunkIndex).forEach([this](const GlobalIndex& stencilIndex)
      {
        addToLazyMeshUpdateQueue(stencilIndex);
      });
}

void ChunkManager::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  /*
    NOTE: An alternative, more general algorithm is to consider the bounding box of the
    touched blocks, plus one layer of blocks along each direction. The box of influenced
    chunks can then be computed directly from this.
  */

  addToForceMeshUpdateQueue(chunkIndex);

  std::vector<Direction> updateDirections{};
  for (Direction direction : Directions())
    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, direction))
      updateDirections.push_back(direction);

  EN_ASSERT(updateDirections.size() <= 3, "Too many update directions for a single block update!");

  // Update neighbors in cardinal directions
  for (Direction direction : updateDirections)
    addToForceMeshUpdateQueue(chunkIndex + GlobalIndex::Dir(direction));

  // Queue edge neighbor for updating
  if (updateDirections.size() == 2)
    addToLazyMeshUpdateQueue(chunkIndex + GlobalIndex::Dir(updateDirections[0]) + GlobalIndex::Dir(updateDirections[1]));

  // Queue edge/corner neighbors for updating
  if (updateDirections.size() == 3)
  {
    GlobalIndex cornerNeighborIndex = chunkIndex;
    for (int i = 0; i < 3; ++i)
    {
      int j = (i + 1) % 3;
      GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::Dir(updateDirections[i]) + GlobalIndex::Dir(updateDirections[j]);
      addToLazyMeshUpdateQueue(edgeNeighborIndex);

      cornerNeighborIndex += GlobalIndex::Dir(updateDirections[i]);
    }
    addToLazyMeshUpdateQueue(cornerNeighborIndex);
  }
}

void ChunkManager::uploadMeshes(Engine::Threads::UnorderedSet<ChunkDrawCommand>& commandQueue, std::unique_ptr<Engine::MultiDrawIndexedArray<ChunkDrawCommand>>& multiDrawArray)
{
  std::unordered_set<ChunkDrawCommand> drawCommands = commandQueue.removeAll();
  while (!drawCommands.empty())
  {
    auto nodeHandle = drawCommands.extract(drawCommands.begin());
    ChunkDrawCommand drawCommand = std::move(nodeHandle.value());

    multiDrawArray->remove(drawCommand.id());
    if (m_ChunkContainer.chunks().contains(drawCommand.id()))
      multiDrawArray->add(std::move(drawCommand));
  }
}

std::pair<bool, bool> ChunkManager::getOnChunkBlockData(BlockData& blockData, const GlobalIndex& chunkIndex, bool getLighting) const
{
  auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
  if (!chunk)
    return { true, false };

  BlockBox fillSection(0, Chunk::Size());
  blockData.fill(fillSection, chunk, Chunk::Bounds(), getLighting);
  return { false, !chunk->composition() };
}

void ChunkManager::getCardinalNeighborBlockData(BlockData& blockData, const GlobalIndex& chunkIndex, bool getLighting) const
{
  for (const auto& [side, faceInterior] : FaceInteriors(BlockData::Bounds()))
  {
    auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(side));
    if (!neighbor)
      continue;

    BlockBox neighborFace = Chunk::Bounds().face(!side);
    blockData.fill(faceInterior, neighbor, neighborFace, getLighting);
  }
}

void ChunkManager::getEdgeNeighborBlockData(BlockData& blockData, const GlobalIndex& chunkIndex, bool getLighting) const
{
  for (const auto& [sideA, sideB, edgeInterior] : EdgeInteriors(BlockData::Bounds()))
  {
    auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(sideA) + GlobalIndex::Dir(sideB));
    if (!neighbor)
      continue;

    BlockBox neighborEdge = Chunk::Bounds().edge(!sideA, !sideB);
    blockData.fill(edgeInterior, neighbor, neighborEdge, getLighting);
  }
}

void ChunkManager::getCornerNeighborBlockData(BlockData& blockData, const GlobalIndex& chunkIndex, bool getLighting) const
{
  for (const auto& [offset, corner] : Corners(BlockData::Bounds()))
  {
    auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + static_cast<GlobalIndex>(offset));
    if (!neighbor)
      continue;

    BlockBox neighborCorner = Chunk::Bounds().corner(-offset);
    blockData.fill(corner, neighbor, neighborCorner, getLighting);
  }
}

void ChunkManager::meshChunk(const GlobalIndex& chunkIndex)
{
  BlockData& blockData = tl_BlockData;
  blockData.composition.fill(Block::Type::Null);
  blockData.lighting.fill(Block::Light());

  auto [chunkWasUnloaded, chunkWasEmpty] = getOnChunkBlockData(blockData, chunkIndex);
  if (chunkWasUnloaded || chunkWasEmpty)
  {
    m_OpaqueCommandQueue.emplace(chunkIndex, false);
    m_TransparentCommandQueue.emplace(chunkIndex, false);
    return;
  }

  EN_PROFILE_FUNCTION();

  getCardinalNeighborBlockData(blockData, chunkIndex);
  getEdgeNeighborBlockData(blockData, chunkIndex);
  getCornerNeighborBlockData(blockData, chunkIndex);

  ChunkDrawCommand opaqueDraw(chunkIndex, true);
  ChunkDrawCommand transparentDraw(chunkIndex, false);
  Chunk::Bounds().forEach([&blockData, &opaqueDraw, &transparentDraw](const BlockIndex& blockIndex)
    {
      Block::Type blockType = blockData.composition(blockIndex);

      if (blockType == Block::Type::Air)
        return;

      uint8_t enabledFaces = 0;
      bool blockIsTransparent = Block::HasTransparency(blockType);
      ChunkDrawCommand& draw = blockIsTransparent ? transparentDraw : opaqueDraw;
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

          BlockIndex vertexPosition = blockIndex + ChunkVertex::GetOffset(face, quadIndex);
          BlockBox lightingStencil = BlockBox(-1, 1) + vertexPosition;
          lightingStencil.forEach([&blockData, &totalSunlight, &transparentNeighbors](const BlockIndex& lightIndex)
            {
              if (!Block::HasTransparency(blockData.composition(lightIndex)))
                return;

              totalSunlight += blockData.lighting(lightIndex).sunlight();
              transparentNeighbors++;
            });

          sunlight[quadIndex] = totalSunlight / std::max(transparentNeighbors, 1);
        }

        // Calculate ambient occlusion
        std::array<int, 4> quadAmbientOcclusion{};
        Block::Texture quadTexture = Block::GetTexture(blockType, face);
        if (!Block::HasTransparency(quadTexture))
          for (int quadIndex = 0; quadIndex < 4; ++quadIndex)
          {
            Axis u = AxisOf(face);
            Axis v = Cycle(u);
            Axis w = Cycle(v);

            Direction edgeADir = ToDirection(v, ChunkVertex::GetOffset(face, quadIndex)[v]);
            Direction edgeBDir = ToDirection(w, ChunkVertex::GetOffset(face, quadIndex)[w]);

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

      if (enabledFaces != 0)
        draw.addVoxel(blockIndex, enabledFaces);
    });
  opaqueDraw.setIndices();

  m_OpaqueCommandQueue.insert(std::move(opaqueDraw));
  m_TransparentCommandQueue.insert(std::move(transparentDraw));
}

void ChunkManager::updateLighting(const GlobalIndex& chunkIndex)
{
  static constexpr int8_t attenuation = 1;

  BlockData& blockData = tl_BlockData;
  blockData.composition.fill(Block::Type::Null);
  blockData.lighting.fill(Block::Light());

  auto [chunkWasUnloaded, chunkWasEmpty] = getOnChunkBlockData(blockData, chunkIndex, false);
  if (chunkWasUnloaded)
    return;

  EN_PROFILE_FUNCTION();

  // Only need data from cardinal neighbors for lighting updates
  getCardinalNeighborBlockData(blockData, chunkIndex);

  // Perform initial propogation of sunlight downward until light hits opaque block
  ArrayRect<blockIndex_t, blockIndex_t, 0, Chunk::Size()> attenuatedSunlightExtents(Chunk::Size());
  BlockData::Bounds().faceInterior(Direction::Top).forEach([&blockData, &attenuatedSunlightExtents](const BlockIndex& blockIndex)
    {
      BlockIndex propogationIndex = blockIndex;
      if (blockData.lighting(propogationIndex) != Block::Light::MaxValue())
        return;

      for (propogationIndex.k = blockIndex.k - 1; propogationIndex.k >= Chunk::Bounds().min.k; --propogationIndex.k)
      {
        if (!Block::HasTransparency(blockData.composition(propogationIndex)))
          break;

        blockData.lighting(propogationIndex) = Block::Light::MaxValue();
      }

      blockIndex_t i = propogationIndex.i;
      blockIndex_t j = propogationIndex.j;
      blockIndex_t k = propogationIndex.k + 1;

      if (j - 1 > 0)
        attenuatedSunlightExtents[i][j - 1] = std::min(attenuatedSunlightExtents[i][j - 1], k);
      if (j + 1 < Chunk::Size())
        attenuatedSunlightExtents[i][j + 1] = std::min(attenuatedSunlightExtents[i][j + 1], k);
      if (i - 1 > 0)
        attenuatedSunlightExtents[i - 1][j] = std::min(attenuatedSunlightExtents[i - 1][j], k);
      if (i + 1 < Chunk::Size())
        attenuatedSunlightExtents[i + 1][j] = std::min(attenuatedSunlightExtents[i + 1][j], k);
    });

  // Light unlit blocks neighboring sunlight with attenuated sunlight value and add them to the propogation stack
  std::array<std::stack<BlockIndex>, Block::Light::MaxValue() + 1> sunlight;
  attenuatedSunlightExtents.bounds().forEach([&blockData, &sunlight, &attenuatedSunlightExtents](const IVec2<blockIndex_t>& index)
    {
      static constexpr int8_t attenuatedIntensity = Block::Light::MaxValue() - attenuation;

      for (BlockIndex blockIndex(index, attenuatedSunlightExtents(index)); blockIndex.k < Chunk::Size(); ++blockIndex.k)
      {
        if (!Block::HasTransparency(blockData.composition(blockIndex)) || blockData.lighting(blockIndex) == Block::Light::MaxValue())
          continue;

        blockData.lighting(blockIndex) = attenuatedIntensity;
        sunlight[attenuatedIntensity].push(blockIndex);
      }
    });

  // Add lit blocks in neighboring chunk to propogation stack
  for (Direction direction = Direction::West; direction < Direction::Top; ++direction)
  {
    BlockBox faceInterior = BlockData::Bounds().faceInterior(direction);
    faceInterior.forEach([&blockData, &sunlight](const BlockIndex& blockIndex)
      {
        if (Block::HasTransparency(blockData.composition(blockIndex)))
          sunlight[blockData.lighting(blockIndex).sunlight()].push(blockIndex);
      });
  }

  // Propogate attenuated sunlight
  for (int8_t intensity = Block::Light::MaxValue(); intensity > 0; --intensity)
    while (!sunlight[intensity].empty())
    {
      BlockIndex lightIndex = sunlight[intensity].top();
      sunlight[intensity].pop();

      for (Direction direction : Directions())
      {
        BlockIndex lightNeighbor = lightIndex + BlockIndex::Dir(direction);
        if (!Chunk::Bounds().encloses(lightNeighbor) || !Block::HasTransparency(blockData.composition(lightNeighbor)))
          continue;

        int8_t neighborIntensity = intensity - attenuation;
        if (neighborIntensity <= blockData.lighting(lightNeighbor).sunlight())
          continue;

        blockData.lighting(lightNeighbor) = neighborIntensity;
        sunlight[neighborIntensity].push(lightNeighbor);
      }
    }

  ArrayBox<Block::Light, blockIndex_t, 0, Chunk::Size()> newLighting;
  if (blockData.lighting.anyOf(Chunk::Bounds(), [](Block::Light blockLight) { return blockLight != Block::Light::MaxValue(); }))
  {
    newLighting = ArrayBox<Block::Light, blockIndex_t, 0, Chunk::Size()>(AllocationPolicy::ForOverWrite);
    newLighting.fill(Chunk::Bounds(), blockData.lighting, Chunk::Bounds());
  }

  auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
  if (!chunk)
    return;

  std::unordered_set<GlobalIndex> additionalLightingUpdates;

  for (const auto& [side, faceInterior] : FaceInteriors(Chunk::Bounds()))
    if (!newLighting.contentsEqual(faceInterior, chunk->lighting(), faceInterior))
      additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(side));

  for (const auto& [sideA, sideB, edgeInterior] : EdgeInteriors(Chunk::Bounds()))
  {
    if (newLighting.contentsEqual(edgeInterior, chunk->lighting(), edgeInterior))
      continue;

    additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(sideA));
    additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(sideB));
    additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(sideA) + GlobalIndex::Dir(sideB));
  }

  for (const auto& [offset, corner] : Corners(Chunk::Bounds()))
  {
    if (newLighting.contentsEqual(corner, chunk->lighting(), corner))
      continue;

    GlobalIndex cornerNeighbor = chunkIndex + static_cast<GlobalIndex>(offset);
    GlobalBox updateBox = GlobalBox::VoidBox().expandToEnclose(chunkIndex).expandToEnclose(cornerNeighbor);
    updateBox.forEach([&additionalLightingUpdates](const GlobalIndex& updateIndex)
      {
        additionalLightingUpdates.insert(updateIndex);
      });
  }

  chunk->setLighting(std::move(newLighting));

  additionalLightingUpdates.erase(chunkIndex);
  for (const GlobalIndex& updateIndex : additionalLightingUpdates)
    addToLightingUpdateQueue(updateIndex);

  addToLazyMeshUpdateQueue(chunkIndex);
}

void ChunkManager::lightingPacket(const GlobalIndex& chunkIndex)
{
  if (m_ChunkContainer.hasBoundaryNeighbors(chunkIndex))
    return;

  updateLighting(chunkIndex);
}

void ChunkManager::lazyMeshingPacket(const GlobalIndex& chunkIndex)
{
  if (m_ChunkContainer.hasBoundaryNeighbors(chunkIndex))
    return;

  meshChunk(chunkIndex);
  auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
  if (chunk)
    chunk->update();
}

void ChunkManager::forceMeshingPacket(const GlobalIndex& chunkIndex)
{
  EN_PROFILE_FUNCTION();

  if (!m_ChunkContainer.chunks().contains(chunkIndex))
    generateNewChunk(chunkIndex);

  meshChunk(chunkIndex);
  auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
  if (chunk)
    chunk->update();
}
