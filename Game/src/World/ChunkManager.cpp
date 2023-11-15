#include "GMpch.h"
#include "ChunkManager.h"
#include "Terrain.h"
#include "Player/Player.h"
#include "Indexing/Operations.h"

ChunkManager::ChunkManager()
  : m_ThreadPool(std::make_shared<eng::thread::ThreadPool>(1)),
    m_LoadWork(m_ThreadPool, eng::thread::Priority::Normal),
    m_CleanWork(m_ThreadPool, eng::thread::Priority::High),
    m_LightingWork(m_ThreadPool, eng::thread::Priority::Normal),
    m_LazyMeshingWork(m_ThreadPool, eng::thread::Priority::Normal),
    m_ForceMeshingWork(m_ThreadPool, eng::thread::Priority::Immediate) {}

ChunkManager::~ChunkManager()
{
  m_ThreadPool->shutdown();
}

void ChunkManager::initialize()
{
  s_Shader = eng::Shader::Create("assets/shaders/Chunk.glsl");
  s_LightUniform = eng::Uniform::Create(2, sizeof(LightUniforms));
  s_TextureArray = block::getTextureArray();

  s_SSBO = eng::StorageBuffer::Create(eng::StorageBuffer::Type::SSBO, c_StorageBufferBinding);
  s_SSBO->set(nullptr, c_StorageBufferSize);
  s_SSBO->bind();

  m_OpaqueMultiDrawArray = eng::MultiDrawIndexedArray<ChunkDrawCommand>(s_VertexBufferLayout);
  m_TransparentMultiDrawArray = eng::MultiDrawIndexedArray<ChunkDrawCommand>(s_VertexBufferLayout);

  LightUniforms lightUniforms;
  lightUniforms.sunIntensity = 1.0f;
  s_LightUniform->set(&lightUniforms, sizeof(LightUniforms));
}

void ChunkManager::render()
{
  ENG_PROFILE_FUNCTION();

  m_ThreadPool->queuedTasks();

  eng::math::Mat4 viewProjection = eng::scene::CalculateViewProjection(eng::scene::ActiveCamera());
  eng::EnumArray<eng::math::Vec4, eng::math::FrustumPlane> frustumPlanes = eng::math::calculateViewFrustumPlanes(viewProjection);

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
  static constexpr length_t chunkSphereRadius = std::numbers::sqrt3_v<length_t> * Chunk::Length() / 2;
  for (eng::math::Vec4& plane : frustumPlanes)
  {
    length_t planeNormalMag = glm::length(eng::math::Vec3(plane));
    plane.w += chunkSphereRadius * planeNormalMag;
  }

  eng::math::Vec3 playerCameraPosition = player::cameraPosition();
  GlobalIndex originIndex = player::originIndex();

  s_Shader->bind();
  s_TextureArray->bind(c_TextureSlot);

  {
    eng::render::command::setDepthWriting(true);
    eng::render::command::setUseDepthOffset(false);

    i32 commandCount = m_OpaqueMultiDrawArray.partition([&originIndex, &frustumPlanes](const GlobalIndex& chunkIndex)
    {
      eng::math::Vec3 anchorPosition = Chunk::AnchorPosition(chunkIndex, originIndex);
      eng::math::Vec3 chunkCenter = Chunk::Center(anchorPosition);
      return isInRange(chunkIndex, originIndex, c_RenderDistance) && eng::math::isInFrustum(chunkCenter, frustumPlanes);
    });

    std::vector<eng::math::Float4> storageBufferData;
    storageBufferData.reserve(commandCount);
    const std::vector<ChunkDrawCommand>& drawCommands = m_OpaqueMultiDrawArray.getDrawCommandBuffer();
    for (i32 i = 0; i < commandCount; ++i)
    {
      const GlobalIndex& chunkIndex = drawCommands[i].id();
      eng::math::Vec3 chunkAnchor = Chunk::AnchorPosition(chunkIndex, originIndex);
      storageBufferData.emplace_back(chunkAnchor, 0);
    }

    u32 bufferDataSize = eng::arithmeticCast<u32>(storageBufferData.size() * sizeof(eng::math::Float4));
    if (bufferDataSize > c_StorageBufferSize)
      ENG_ERROR("Chunk anchor data exceeds SSBO size!");

    m_OpaqueMultiDrawArray.bind();
    s_SSBO->update(storageBufferData.data(), 0, bufferDataSize);
    eng::render::command::multiDrawIndexed(drawCommands.data(), commandCount, sizeof(ChunkDrawCommand));
  }

  {
    eng::render::command::setBlending(true);
    eng::render::command::setFaceCulling(false);
    eng::render::command::setDepthWriting(false);
    eng::render::command::setUseDepthOffset(true);
    eng::render::command::setDepthOffset(-1.0f, -1.0f);

    i32 commandCount = m_TransparentMultiDrawArray.partition([&originIndex, &frustumPlanes](const GlobalIndex& chunkIndex)
    {
      eng::math::Vec3 anchorPosition = Chunk::AnchorPosition(chunkIndex, originIndex);
      eng::math::Vec3 chunkCenter = Chunk::Center(anchorPosition);
      return isInRange(chunkIndex, originIndex, c_RenderDistance) && eng::math::isInFrustum(chunkCenter, frustumPlanes);
    });
    m_TransparentMultiDrawArray.sort(commandCount, [&originIndex, &playerCameraPosition](const GlobalIndex& chunk)
    {
      // NOTE: Maybe measure min distance to chunk faces instead
      eng::math::Vec3 chunkCenter = Chunk::Center(Chunk::AnchorPosition(chunk, originIndex));
      length_t dist = glm::length2(chunkCenter - playerCameraPosition);
      return dist;
    }, eng::SortPolicy::Descending);
    m_TransparentMultiDrawArray.amend(commandCount, [&originIndex, &playerCameraPosition](ChunkDrawCommand& drawCommand)
    {
      bool orderModified = drawCommand.sort(originIndex, playerCameraPosition);
      return orderModified;
    });

    std::vector<eng::math::Float4> storageBufferData;
    storageBufferData.reserve(commandCount);
    const std::vector<ChunkDrawCommand>& drawCommands = m_TransparentMultiDrawArray.getDrawCommandBuffer();
    for (i32 i = 0; i < commandCount; ++i)
    {
      const GlobalIndex& chunkIndex = drawCommands[i].id();
      eng::math::Vec3 chunkAnchor = Chunk::AnchorPosition(chunkIndex, originIndex);
      storageBufferData.emplace_back(chunkAnchor, 0);
    }

    u32 bufferDataSize = eng::arithmeticCast<u32>(storageBufferData.size() * sizeof(eng::math::Float4));
    if (bufferDataSize > c_StorageBufferSize)
      ENG_ERROR("Chunk anchor data exceeds SSBO size!");

    m_TransparentMultiDrawArray.bind();
    s_SSBO->update(storageBufferData.data(), 0, bufferDataSize);
    eng::render::command::multiDrawIndexed(drawCommands.data(), commandCount, sizeof(ChunkDrawCommand));
  }
}

void ChunkManager::update()
{
  ENG_PROFILE_FUNCTION();

  m_ForceMeshingWork.waitAndDiscardSaved();

  if (!m_OpaqueCommandQueue.empty())
    uploadMeshes(m_OpaqueCommandQueue, m_OpaqueMultiDrawArray);
  if (!m_TransparentCommandQueue.empty())
    uploadMeshes(m_TransparentCommandQueue, m_TransparentMultiDrawArray);
}

void ChunkManager::loadNewChunks()
{
  using namespace std::chrono_literals;

  static std::future<void> future;
  static std::chrono::steady_clock::time_point lastSearchTimePoint;
  static constexpr std::chrono::duration<seconds> searchInterval = 25ms;

  if (m_LoadWork.queuedTasks() > 0)
    return;

  if (m_LoadWork.savedResults() > 0)
    m_LoadWork.waitAndDiscardSaved();

  std::chrono::duration<seconds> timeSinceLastSearch = std::chrono::steady_clock::now() - lastSearchTimePoint;
  if (timeSinceLastSearch < searchInterval || (future.valid() && !eng::thread::isReady(future)))
    return;

  future = m_ThreadPool->submit(eng::thread::Priority::High, [this]()
  {
    std::unordered_set<GlobalIndex> newChunkIndices = m_ChunkContainer.findAllLoadableIndices();

    // Load First chunk if none exist
    if (newChunkIndices.empty() && m_ChunkContainer.chunks().empty())
      newChunkIndices.insert(player::originIndex());

    for (const GlobalIndex& newChunkIndex : newChunkIndices)
      m_LoadWork.submitAndSaveResult(newChunkIndex, &ChunkManager::generateNewChunk, this, newChunkIndex);
  });

  lastSearchTimePoint = std::chrono::steady_clock::now();
}

void ChunkManager::clean()
{
  using namespace std::chrono_literals;

  static std::future<void> future;
  static GlobalIndex previousPlayerOriginIndex;
  static std::chrono::steady_clock::time_point lastSearchTimePoint;
  static constexpr std::chrono::duration<seconds> searchInterval = 50ms;

  if (m_CleanWork.queuedTasks() > 0)
    return;

  if (m_CleanWork.savedResults() > 0)
    m_CleanWork.waitAndDiscardSaved();

  std::chrono::duration<seconds> timeSinceLastSearch = std::chrono::steady_clock::now() - lastSearchTimePoint;
  if (timeSinceLastSearch < searchInterval || previousPlayerOriginIndex == player::originIndex() || (future.valid() && !eng::thread::isReady(future)))
    return;

  future = m_ThreadPool->submit(eng::thread::Priority::High, [this]()
  {
    GlobalIndex originIndex = player::originIndex();
    std::vector<GlobalIndex> chunksMarkedForDeletion = m_ChunkContainer.chunks().getKeys([&originIndex](const GlobalIndex& chunkIndex)
    {
      return !isInRange(chunkIndex, originIndex, c_UnloadDistance);
    });

    for (const GlobalIndex& chunkIndex : chunksMarkedForDeletion)
      m_CleanWork.submitAndSaveResult(chunkIndex, &ChunkManager::eraseChunk, this, chunkIndex);
  });

  previousPlayerOriginIndex = player::originIndex();
  lastSearchTimePoint = std::chrono::steady_clock::now();
}

std::shared_ptr<const Chunk> ChunkManager::getChunk(const LocalIndex& chunkIndex) const
{
  GlobalIndex originChunk = player::originIndex();
  return m_ChunkContainer.chunks().get(GlobalIndex(originChunk.i + chunkIndex.i, originChunk.j + chunkIndex.j, originChunk.k + chunkIndex.k));
}

void ChunkManager::placeBlock(GlobalIndex chunkIndex, BlockIndex blockIndex, eng::math::Direction face, block::Type blockType)
{
  static constexpr blockIndex_t endOfChunk = Chunk::Size() - 1;

  if (blockNeighborIsInAnotherChunk(blockIndex, face))
  {
    chunkIndex += GlobalIndex::Dir(face);
    blockIndex -= endOfChunk * BlockIndex::Dir(face);
  }
  else
    blockIndex += BlockIndex::Dir(face);

  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    return;

  bool validBlockPlacement = chunk->composition().setIf(blockIndex, blockType, [](block::Type containedBlockType)
  {
    return !containedBlockType.hasCollision();
  });

  // If trying to place a block in a space occupied by another block with collision, do nothing and warn
  if (!validBlockPlacement)
  {
    ENG_WARN("Invalid block placement!");
    return;
  }

  if (!blockType.hasTransparency())
    addToLightingUpdateQueue(chunkIndex);
  sendBlockUpdate(chunkIndex, blockIndex);
}

void ChunkManager::removeBlock(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    return;
  block::Type removedBlock = chunk->composition().replace(blockIndex, block::ID::Air);

  if (!removedBlock.hasTransparency())
  {
    // Get estimate of light value of effected block for immediate meshing
    i8 lightEstimate = 0;
    for (eng::math::Direction direction : eng::math::Directions())
    {
      BlockIndex blockNeighbor = blockIndex + BlockIndex::Dir(direction);
      if (!Chunk::Bounds().encloses(blockNeighbor) || !chunk->composition().get(blockNeighbor).hasTransparency())
        continue;

      lightEstimate = std::max(lightEstimate, chunk->lighting().get(blockNeighbor).sunlight());
    }

    // This is necessary to ensure non-identical diffs of chunk boundary so that lighting updates get propogate to neighboring chunks
    lightEstimate = std::max(0, lightEstimate - 2);

    chunk->lighting().set(blockIndex, lightEstimate);
    addToLightingUpdateQueue(chunkIndex);
  }
  sendBlockUpdate(chunkIndex, blockIndex);
}

void ChunkManager::loadChunk(const GlobalIndex& chunkIndex, block::ID blockType)
{
  std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(chunkIndex);
  if (blockType != block::ID::Air)
  {
    chunk->setComposition(BlockArrayBox<block::Type>(Chunk::Bounds(), blockType));
    chunk->setLighting(BlockArrayBox<block::Light>(Chunk::Bounds(), eng::AllocationPolicy::DefaultInitialize));
  }

  m_ChunkContainer.insert(chunkIndex, std::move(chunk));
}

void ChunkManager::initializeLODs()
{
  ENG_PROFILE_FUNCTION();

  std::vector<lod::Octree::Node*> leaves{};

  bool treeModified = true;
  while (treeModified)
  {
    leaves = m_LODTree.getLeaves();
    treeModified = splitAndCombineLODs(leaves);
  }

  // Generate meshes for all LODs
  leaves = m_LODTree.getLeaves();
  for (lod::Octree::Node* leaf : leaves)
  {
    lod::generateMesh(leaf);

    if (leaf->data->primaryMesh.vertices.size() > 0)
      lod::updateMesh(m_LODTree, leaf);
  }
}

void ChunkManager::renderLODs()
{
  ENG_PROFILE_FUNCTION();

  std::vector<lod::Octree::Node*> leaves = m_LODTree.getLeaves();
  eng::math::Mat4 viewProjection = eng::scene::CalculateViewProjection(eng::scene::ActiveCamera());

  eng::EnumArray<eng::math::Vec4, eng::math::FrustumPlane> frustumPlanes = eng::math::calculateViewFrustumPlanes(viewProjection);
  eng::EnumArray<eng::math::Vec4, eng::math::FrustumPlane> shiftedFrustumPlanes = frustumPlanes;
  eng::EnumArray<length_t, eng::math::FrustumPlane> planeNormalMags;

  for (eng::math::FrustumPlane plane : eng::math::FrustumPlanes())
    planeNormalMags[plane] = glm::length(static_cast<eng::math::Vec3>(frustumPlanes[plane]));

  lod::MeshData::BindBuffers();
  for (lod::Octree::Node* leaf : leaves)
    if (leaf->data->primaryMesh.vertexArray != nullptr)
    {
      // Shift each plane by distance equal to radius of sphere that circumscribes LOD
      static constexpr float sqrt3 = 1.732050807568877f;
      const length_t LODSphereRadius = sqrt3 * leaf->length() / 2;
      for (eng::math::FrustumPlane plane : eng::math::FrustumPlanes())
        shiftedFrustumPlanes[plane].w = frustumPlanes[plane].w + LODSphereRadius * planeNormalMags[plane];

      if (eng::math::isInFrustum(leaf->center(), shiftedFrustumPlanes) && !isInRange(leaf->anchor, player::originIndex(), c_RenderDistance - 1))
        lod::draw(leaf);
    }
}

void ChunkManager::manageLODs()
{
  ENG_PROFILE_FUNCTION();

  std::vector<lod::Octree::Node*> leaves = m_LODTree.getLeaves();
  bool treeModified = splitAndCombineLODs(leaves);

  if (treeModified)
  {
    std::vector<lod::Octree::Node*> leaves = m_LODTree.getLeaves();
    for (lod::Octree::Node* leaf : leaves)
    {
      if (!leaf->data->meshGenerated)
        lod::generateMesh(leaf);

      if (leaf->data->needsUpdate && leaf->data->primaryMesh.vertices.size() > 0)
        lod::updateMesh(m_LODTree, leaf);
    }
  }
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

void ChunkManager::addToMeshRemovalQueue(const GlobalIndex& chunkIndex)
{
  m_OpaqueCommandQueue.emplace(chunkIndex, false);
  m_TransparentCommandQueue.emplace(chunkIndex, false);
}

std::shared_ptr<Chunk> ChunkManager::generateNewChunk(const GlobalIndex& chunkIndex)
{
  ENG_PROFILE_FUNCTION();

  std::shared_ptr<Chunk> chunk = terrain::generateNew(chunkIndex);

  bool insertionSuccess = m_ChunkContainer.insert(chunkIndex, std::move(chunk));
  if (insertionSuccess)
    Chunk::Stencil(chunkIndex).forEach([this](const GlobalIndex& stencilIndex)
    {
      addToLazyMeshUpdateQueue(stencilIndex);
    });
  return chunk;
}

void ChunkManager::eraseChunk(const GlobalIndex& chunkIndex)
{
  if (!isInRange(chunkIndex, player::originIndex(), c_UnloadDistance))
  {
    // Order is important here to prevent memory leaks
    m_ChunkContainer.erase(chunkIndex);
    addToMeshRemovalQueue(chunkIndex);
  }
}

void ChunkManager::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  affectedChunks(BlockBox(blockIndex, blockIndex)).forEach([this, &chunkIndex](const LocalIndex& localIndex)
  {
    GlobalIndex neighborIndex = chunkIndex + localIndex.upcast<globalIndex_t>();

    // Chunk and its face neighbors get queued for immediate meshing
    if (localIndex.l1Norm() <= 1)
      addToForceMeshUpdateQueue(neighborIndex);
    else
      addToLazyMeshUpdateQueue(neighborIndex);
  });
}

void ChunkManager::uploadMeshes(eng::thread::UnorderedSet<ChunkDrawCommand>& commandQueue, eng::MultiDrawIndexedArray<ChunkDrawCommand>& multiDrawArray)
{
  std::unordered_set<ChunkDrawCommand> drawCommands = commandQueue.removeAll();
  while (!drawCommands.empty())
  {
    auto nodeHandle = drawCommands.extract(drawCommands.begin());
    ChunkDrawCommand drawCommand = std::move(nodeHandle.value());

    multiDrawArray.remove(drawCommand.id());
    if (m_ChunkContainer.chunks().contains(drawCommand.id()))
      multiDrawArray.add(std::move(drawCommand));
  }
}



struct BlockData
{
  BlockArrayBox<block::Type> composition;
  BlockArrayBox<block::Light> lighting;

  BlockData()
    : composition(Bounds(), eng::AllocationPolicy::Deferred), lighting(Bounds(), eng::AllocationPolicy::Deferred) {}

  static constexpr BlockBox Bounds() { return BlockBox(-1, Chunk::Size()); }
};

void ChunkManager::meshChunk(const Chunk& chunk)
{
  const GlobalIndex& chunkIndex = chunk.globalIndex();

  // Return early if chunk is entirely air
  if (!chunk.composition())
  {
    addToMeshRemovalQueue(chunkIndex);
    return;
  }
  ENG_PROFILE_FUNCTION();

  BlockData blockData;
  blockData.composition = m_ChunkContainer.retrieveTypeData(chunk, { BlockData::Bounds() });
  blockData.lighting = m_ChunkContainer.retrieveLightingData(chunk, { BlockData::Bounds() });

  ChunkDrawCommand opaqueDraw(chunkIndex, false);
  ChunkDrawCommand transparentDraw(chunkIndex, true);
  Chunk::Bounds().forEach([&blockData, &opaqueDraw, &transparentDraw](const BlockIndex& blockIndex)
  {
    block::Type blockType = blockData.composition(blockIndex);

    if (blockType == block::ID::Air)
      return;

    eng::math::DirectionBitMask enabledFaces;
    ChunkDrawCommand& draw = blockType.hasTransparency() ? transparentDraw : opaqueDraw;
    for (eng::math::Direction face : eng::math::Directions())
    {
      BlockIndex cardinalIndex = blockIndex + BlockIndex::Dir(face);
      block::Type cardinalNeighbor = blockData.composition(cardinalIndex);
      if (cardinalNeighbor == blockType || (!blockType.hasTransparency() && !cardinalNeighbor.hasTransparency()))
        continue;

      enabledFaces.set(face);

      // Calculate lighting
      std::array<i32, 4> sunlight{};
      for (i32 quadIndex = 0; quadIndex < 4; ++quadIndex)
      {
        i32 transparentNeighbors = 0;
        i32 totalSunlight = 0;

        BlockIndex vertexPosition = blockIndex + ChunkVertex::GetOffset(face, quadIndex);
        BlockBox lightingStencil = BlockBox(-1, 0) + vertexPosition;
        lightingStencil.forEach([&blockData, &totalSunlight, &transparentNeighbors](const BlockIndex& lightIndex)
        {
          if (!blockData.composition(lightIndex).hasTransparency())
            return;

          totalSunlight += blockData.lighting(lightIndex).sunlight();
          transparentNeighbors++;
        });

        sunlight[quadIndex] = totalSunlight / std::max(transparentNeighbors, 1);
      }

      // Calculate ambient occlusion
      std::array<i32, 4> quadAmbientOcclusion{};
      if (!blockType.hasTransparency())
        for (i32 quadIndex = 0; quadIndex < 4; ++quadIndex)
        {
          eng::math::Axis u = axisOf(face);
          eng::math::Axis v = cycle(u);
          eng::math::Axis w = cycle(v);

          eng::math::Direction edgeADir = toDirection(v, ChunkVertex::GetOffset(face, quadIndex)[v]);
          eng::math::Direction edgeBDir = toDirection(w, ChunkVertex::GetOffset(face, quadIndex)[w]);

          BlockIndex edgeA = blockIndex + BlockIndex::Dir(face) + BlockIndex::Dir(edgeADir);
          BlockIndex edgeB = blockIndex + BlockIndex::Dir(face) + BlockIndex::Dir(edgeBDir);
          BlockIndex corner = blockIndex + BlockIndex::Dir(face) + BlockIndex::Dir(edgeADir) + BlockIndex::Dir(edgeBDir);

          bool edgeAIsOpaque = !blockData.composition(edgeA).hasTransparency();
          bool edgeBIsOpaque = !blockData.composition(edgeB).hasTransparency();
          bool cornerIsOpaque = !blockData.composition(corner).hasTransparency();
          quadAmbientOcclusion[quadIndex] = edgeAIsOpaque && edgeBIsOpaque ? 3 : edgeAIsOpaque + edgeBIsOpaque + cornerIsOpaque;
        }

      draw.addQuad(blockIndex, face, blockType.texture(face), sunlight, quadAmbientOcclusion);
    }

    if (!enabledFaces.empty())
      draw.addVoxel(blockIndex, enabledFaces);
  });

  m_OpaqueCommandQueue.insertOrReplace(std::move(opaqueDraw));
  m_TransparentCommandQueue.insertOrReplace(std::move(transparentDraw));
}

void ChunkManager::updateLighting(Chunk& chunk)
{
  ENG_PROFILE_FUNCTION();

  static constexpr i8 attenuation = 1;
  const GlobalIndex& chunkIndex = chunk.globalIndex();

  BlockData blockData;

  // Only need data from cardinal neighbors for lighting updates
  std::vector<BlockBox> chunkSections = eng::algo::asVector(eng::math::FaceInteriors(BlockData::Bounds()));
  blockData.lighting = m_ChunkContainer.retrieveLightingData(chunk, chunkSections);

  chunkSections.push_back(Chunk::Bounds());
  blockData.composition = m_ChunkContainer.retrieveTypeData(chunk, chunkSections);

  // Perform initial propogation of sunlight downward until light hits opaque block
  BlockArrayRect<blockIndex_t> attenuatedSunlightExtents(Chunk::Bounds2D(), Chunk::Size());
  BlockData::Bounds().faceInterior(eng::math::Direction::Top).forEach([&blockData, &attenuatedSunlightExtents](const BlockIndex& blockIndex)
  {
    BlockIndex propogationIndex = blockIndex;
    if (blockData.lighting(propogationIndex) != block::Light::MaxValue())
      return;

    for (propogationIndex.k = blockIndex.k - 1; propogationIndex.k >= Chunk::Bounds().min.k; --propogationIndex.k)
    {
      if (!blockData.composition(propogationIndex).hasTransparency())
        break;

      blockData.lighting(propogationIndex) = block::Light::MaxValue();
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
  std::array<std::stack<BlockIndex>, block::Light::MaxValue() + 1> sunlight;
  attenuatedSunlightExtents.bounds().forEach([&blockData, &sunlight, &attenuatedSunlightExtents](const eng::math::IVec2<blockIndex_t>& index)
  {
    static constexpr i8 attenuatedIntensity = block::Light::MaxValue() - attenuation;
    for (BlockIndex blockIndex(index, attenuatedSunlightExtents(index)); blockIndex.k < Chunk::Size(); ++blockIndex.k)
    {
      if (!blockData.composition(blockIndex).hasTransparency() || blockData.lighting(blockIndex) == block::Light::MaxValue())
        continue;

      blockData.lighting(blockIndex) = attenuatedIntensity;
      sunlight[attenuatedIntensity].push(blockIndex);
    }
  });

  // Add lit blocks in neighboring chunk to propogation stack
  for (eng::math::Direction direction : eng::math::Directions())
  {
    // Top neighbors already accounted for
    if (direction == eng::math::Direction::Top)
      continue;

    BlockBox faceInterior = BlockData::Bounds().faceInterior(direction);
    faceInterior.forEach([&blockData, &sunlight](const BlockIndex& blockIndex)
    {
      if (blockData.composition(blockIndex).hasTransparency())
        sunlight[blockData.lighting(blockIndex).sunlight()].push(blockIndex);
    });
  }

  // Propogate attenuated sunlight
  for (i8 intensity = block::Light::MaxValue(); intensity > 0; --intensity)
    while (!sunlight[intensity].empty())
    {
      BlockIndex lightIndex = sunlight[intensity].top();
      sunlight[intensity].pop();

      for (eng::math::Direction direction : eng::math::Directions())
      {
        BlockIndex lightNeighbor = lightIndex + BlockIndex::Dir(direction);
        if (!Chunk::Bounds().encloses(lightNeighbor) || !blockData.composition(lightNeighbor).hasTransparency())
          continue;

        i8 neighborIntensity = intensity - attenuation;
        if (neighborIntensity <= blockData.lighting(lightNeighbor).sunlight())
          continue;

        blockData.lighting(lightNeighbor) = neighborIntensity;
        sunlight[neighborIntensity].push(lightNeighbor);
      }
    }

  BlockArrayBox<block::Light> newLighting(Chunk::Bounds(), eng::AllocationPolicy::Deferred);
  if (blockData.lighting.anyOf(Chunk::Bounds(), [](block::Light blockLight) { return blockLight != block::Light::MaxValue(); }))
  {
    newLighting.allocate();
    newLighting.fill(Chunk::Bounds(), blockData.lighting, Chunk::Bounds(), block::Light::MaxValue());
  }

  std::unordered_set<GlobalIndex> additionalLightingUpdates;
  chunk.lighting().readOperation([&chunkIndex, &newLighting, &additionalLightingUpdates](const BlockArrayBox<block::Light>& lighting, const block::Light& defaultValue)
  {
    static constexpr std::array<BlockBox, 26> chunkDecomposition = decomposeBlockBoxBoundary(Chunk::Bounds());
    for (const BlockBox& chunkSection : chunkDecomposition)
      if (!lighting.contentsEqual(chunkSection, newLighting, chunkSection, defaultValue))
        affectedChunks(chunkSection).forEach([&chunkIndex, &additionalLightingUpdates](const LocalIndex& localIndex)
        {
          additionalLightingUpdates.insert(chunkIndex + localIndex.upcast<globalIndex_t>());
        });
  });

  chunk.setLighting(std::move(newLighting));

  additionalLightingUpdates.erase(chunkIndex);
  for (const GlobalIndex& updateIndex : additionalLightingUpdates)
    addToLightingUpdateQueue(updateIndex);

  addToLazyMeshUpdateQueue(chunkIndex);
}

void ChunkManager::lightingPacket(const GlobalIndex& chunkIndex)
{
  if (m_ChunkContainer.hasBoundaryNeighbors(chunkIndex))
    return;

  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    return;

  updateLighting(*chunk);
}

void ChunkManager::lazyMeshingPacket(const GlobalIndex& chunkIndex)
{
  if (m_ChunkContainer.hasBoundaryNeighbors(chunkIndex))
    return;

  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    return;

  meshChunk(*chunk);
  chunk->update();
}

void ChunkManager::forceMeshingPacket(const GlobalIndex& chunkIndex)
{
  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    chunk = generateNewChunk(chunkIndex);

  meshChunk(*chunk);
  chunk->update();
}

bool ChunkManager::splitLODs(std::vector<lod::Octree::Node*>& leaves)
{
  bool treeModified = false;
  for (auto it = leaves.begin(); it != leaves.end();)
  {
    lod::Octree::Node* node = *it;

    if (node->LODLevel() > 0)
    {
      globalIndex_t splitRange = 2 * node->size() - 1 + c_RenderDistance;
      GlobalBox splitRangeBoundingBox(player::originIndex() - splitRange, player::originIndex() + splitRange);

      GlobalBox intersection = GlobalBox::Intersection(splitRangeBoundingBox, node->boundingBox());
      if (intersection.valid())
      {
        m_LODTree.splitNode(node);
        lod::messageNeighbors(m_LODTree, node);

        it = leaves.erase(it);
        treeModified = true;
        continue;
      }
    }

    it++;
  }

  return treeModified;
}

bool ChunkManager::combineLODs(std::vector<lod::Octree::Node*>& leaves)
{
  // Search for nodes to combine
  std::vector<lod::Octree::Node*> cannibalNodes{};
  for (lod::Octree::Node* node : leaves)
    if (node->depth > 0)
    {
      globalIndex_t combineRange = 4 * node->size() - 1 + c_RenderDistance;
      GlobalBox rangeBoundingBox(player::originIndex() - combineRange, player::originIndex() + combineRange);

      if (!GlobalBox::Intersection(rangeBoundingBox, node->parent->boundingBox()).valid())
        cannibalNodes.push_back(node->parent);
    }

  // Combine nodes
  for (lod::Octree::Node* node : cannibalNodes)
  {
    m_LODTree.combineChildren(node);
    lod::messageNeighbors(m_LODTree, node);
  }

  return cannibalNodes.size() > 0;
}

bool ChunkManager::splitAndCombineLODs(std::vector<lod::Octree::Node*>& leaves)
{
  bool nodesSplit = splitLODs(leaves);
  bool nodesCombined = combineLODs(leaves);
  return nodesSplit || nodesCombined;
}
