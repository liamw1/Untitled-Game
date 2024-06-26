#include "GMpch.h"
#include "ChunkManager.h"
#include "Indexing/Operations.h"
#include "Player/Player.h"
#include "World/Terrain.h"

// Rendering
static constexpr i32 c_TextureSlot = 0;
static constexpr i32 c_SSBOBinding = 1;
static constexpr i32 c_LightUniformBinding = 2;
static constexpr u32 c_SSBOSize = eng::math::pow2<u32>(20);
static std::unique_ptr<eng::Shader> s_Shader;
static std::unique_ptr<eng::Uniform> s_LightUniform;
static std::unique_ptr<eng::ShaderBufferStorage> s_SSBO;
static std::shared_ptr<eng::TextureArray> s_TextureArray;
static const eng::mem::BufferLayout s_VertexBufferLayout = {{ eng::mem::DataType::Unsigned, "a_VertexData" },
                                                            { eng::mem::DataType::Unsigned, "a_Lighting"   }};

struct LightUniformData
{
  const f32 maxSunlight = eng::arithmeticUpcast<f32>(block::Light::MaxValue());
  f32 sunIntensity = 1.0f;
};

class DeallocatorPayload
{
  GlobalIndex m_ChunkIndex;
  std::shared_ptr<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>> m_OpaqueAsyncArray;
  std::shared_ptr<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>> m_TransparentAsyncArray;

public:
  DeallocatorPayload() = default;
  DeallocatorPayload(const GlobalIndex& chunkIndex,
                     const std::shared_ptr<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>>& opaqueMultiDrawArray,
                     const std::shared_ptr<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>>& transparentMultiDrawArray)
    : m_ChunkIndex(chunkIndex), m_OpaqueAsyncArray(opaqueMultiDrawArray), m_TransparentAsyncArray(transparentMultiDrawArray) {}

  bool operator==(const DeallocatorPayload& other) const = default;

  void operator()()
  {
    m_OpaqueAsyncArray->removeCommand(m_ChunkIndex);
    m_TransparentAsyncArray->removeCommand(m_ChunkIndex);
  }
};

struct BlockData
{
  BlockArrayBox<block::Type> composition;
  BlockArrayBox<block::Light> lighting;

  BlockData()
    : composition(Bounds(), eng::AllocationPolicy::Deferred), lighting(Bounds(), eng::AllocationPolicy::Deferred) {}

  static constexpr BlockBox Bounds() { return BlockBox(-1, Chunk::Size()); }
};

// TODO: Remove
static BlockArrayBox<block::Light> calculateLighting(const BlockArrayBox<block::Type>& composition)
{
  BlockArrayBox<block::Light> lighting(Chunk::Bounds(), eng::AllocationPolicy::Deferred);
  if (!composition)
    return lighting;

  lighting.allocate();
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
    {
      blockIndex_t k = 0;
      while (k < Chunk::Size() && !composition[i][j][k].hasTransparency())
      {
        lighting[i][j][k] = block::Light(0);
        k++;
      }
      for (; k < Chunk::Size(); ++k)
        lighting[i][j][k] = block::Light(block::Light::MaxValue());
    }
  return lighting;
}



ChunkManager::ChunkManager()
  : m_OpaqueMultiDrawArray(std::make_shared<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>>(s_VertexBufferLayout)),
    m_TransparentMultiDrawArray(std::make_shared<eng::thread::AsyncMultiDrawArray<ChunkDrawCommand>>(s_VertexBufferLayout)),
    m_ThreadPool(std::make_shared<eng::thread::ThreadPool>("Chunk Manager", 0.25)),
    m_LoadWork(m_ThreadPool, eng::thread::Priority::Normal),
    m_CleanWork(m_ThreadPool, eng::thread::Priority::High),
    m_LightingWork(m_ThreadPool, eng::thread::Priority::Normal),
    m_LazyMeshingWork(m_ThreadPool, eng::thread::Priority::Normal),
    m_ForceMeshingWork(m_ThreadPool, eng::thread::Priority::Immediate)
{
  ENG_PROFILE_FUNCTION();

  s_Shader = eng::Shader::Create("assets/shaders/Chunk.glsl");
  s_LightUniform = std::make_unique<eng::Uniform>("Light", c_LightUniformBinding, sizeof(LightUniformData));
  s_TextureArray = block::getTextureArray();
  s_SSBO = std::make_unique<eng::ShaderBufferStorage>(c_SSBOBinding, c_SSBOSize);

  LightUniformData lightUniforms;
  lightUniforms.sunIntensity = 1.0f;
  s_LightUniform->write(lightUniforms);
}

ChunkManager::~ChunkManager()
{
  m_ThreadPool->shutdown();
}

void ChunkManager::render()
{
  ENG_PROFILE_FUNCTION();

  eng::Entity activeCameraEntity = eng::scene::ActiveCamera();
  eng::math::Mat4 viewProjection = eng::scene::CalculateViewProjection(activeCameraEntity);
  eng::EnumArray<eng::math::Vec4, eng::math::FrustumPlane> frustumPlanes = eng::math::calculateViewFrustumPlanes(viewProjection);

  // Shift each plane by distance equal to radius of sphere that circumscribes chunk
  for (eng::math::Vec4& plane : frustumPlanes)
  {
    length_t planeNormalMag = glm::length(eng::math::Vec3(plane));
    plane.w += Chunk::BoundingSphereRadius() * planeNormalMag;
  }

  eng::math::Vec3 cameraPosition = activeCameraEntity.get<eng::component::Transform>().position;
  GlobalIndex originIndex = player::originIndex();

  auto isInViewFrustum = [&frustumPlanes, &originIndex](const GlobalIndex& chunkIndex)
  {
    eng::math::Vec3 chunkCenter = indexCenter(chunkIndex, originIndex);
    return isInRange(chunkIndex, originIndex, param::RenderDistance()) && eng::math::isInFrustum(chunkCenter, frustumPlanes);
  };
  auto chunkDistance = [&originIndex](const GlobalIndex& chunkIndex)
  {
    return (chunkIndex - originIndex).l1Norm();
  };
  auto draw = [&originIndex](const eng::MultiDrawArray<ChunkDrawCommand>& multiDrawArray, uSize commandCount)
  {
    const std::vector<ChunkDrawCommand>& drawCommands = multiDrawArray.getDrawCommandBuffer();

    std::vector<eng::math::Float4> storageBufferData;
    storageBufferData.reserve(commandCount);
    for (uSize i = 0; i < commandCount; ++i)
    {
      const GlobalIndex& chunkIndex = drawCommands[i].id();
      eng::math::Vec3 chunkAnchorPosition = indexPosition(chunkIndex, originIndex);
      storageBufferData.emplace_back(chunkAnchorPosition, 0);
    }
    s_SSBO->write(storageBufferData);

    multiDrawArray.bind();
    eng::render::command::multiDrawIndexed(drawCommands, commandCount);
  };

  s_Shader->bind();
  s_TextureArray->bind(c_TextureSlot);

  eng::render::command::setFaceCulling(true);
  eng::render::command::setDepthWriting(true);
  eng::render::command::setUseDepthOffset(false);
  m_OpaqueMultiDrawArray->drawOperation([&isInViewFrustum, &chunkDistance, &draw](eng::MultiDrawArray<ChunkDrawCommand>& multiDrawArray)
  {
    uSize commandCount = multiDrawArray.partition(isInViewFrustum);
    multiDrawArray.sort(commandCount, chunkDistance, eng::SortPolicy::Ascending);   // Sort opaque meshes front-to-back
    draw(multiDrawArray, commandCount);
  });

  eng::render::command::setBlending(true);
  eng::render::command::setFaceCulling(false);
  eng::render::command::setDepthWriting(false);
  eng::render::command::setUseDepthOffset(true);
  eng::render::command::setDepthOffset(1.0f, 1.0f);
  m_TransparentMultiDrawArray->drawOperation([&isInViewFrustum, &chunkDistance, &draw, &originIndex, &cameraPosition](eng::MultiDrawArray<ChunkDrawCommand>& multiDrawArray)
  {
    uSize commandCount = multiDrawArray.partition(isInViewFrustum);
    multiDrawArray.sort(commandCount, chunkDistance, eng::SortPolicy::Descending);  // Sort transparent meshes back-to-front
    multiDrawArray.modifyIndices(commandCount, [&originIndex, &cameraPosition](ChunkDrawCommand& drawCommand)
    {
      bool orderModified = drawCommand.sort(originIndex, cameraPosition);
      return orderModified;
    });

    draw(multiDrawArray, commandCount);
  });
}

void ChunkManager::update()
{
  ENG_PROFILE_FUNCTION();

  m_ForceMeshingWork.waitAndDiscardSaved();
  m_OpaqueMultiDrawArray->uploadQueuedCommands();
  m_TransparentMultiDrawArray->uploadQueuedCommands();
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
      return !isInRange(chunkIndex, originIndex, param::UnloadDistance());
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



void ChunkManager::addToLightingUpdateQueue(const GlobalIndex& chunkIndex)
{
  m_LightingWork.submit(chunkIndex, &ChunkManager::lightingTask, this, chunkIndex);
}

void ChunkManager::addToLazyMeshUpdateQueue(const GlobalIndex& chunkIndex)
{
  if (m_ForceMeshingWork.contains(chunkIndex))
    return;

  m_LazyMeshingWork.submit(chunkIndex, &ChunkManager::lazyMeshingTask, this, chunkIndex);
}

void ChunkManager::addToForceMeshUpdateQueue(const GlobalIndex& chunkIndex)
{
  m_ForceMeshingWork.submitAndSaveResult(chunkIndex, &ChunkManager::forceMeshingTask, this, chunkIndex);
}

void ChunkManager::removeMeshes(const GlobalIndex& chunkIndex)
{
  m_OpaqueMultiDrawArray->removeCommand(chunkIndex);
  m_TransparentMultiDrawArray->removeCommand(chunkIndex);
}

std::shared_ptr<Chunk> ChunkManager::generateNewChunk(const GlobalIndex& chunkIndex)
{
  ENG_PROFILE_FUNCTION();

  eng::mem::UponDeallocation<DeallocatorPayload, Chunk> chunkAllocator(chunkIndex, m_OpaqueMultiDrawArray, m_TransparentMultiDrawArray);
  std::shared_ptr<Chunk> chunk = std::allocate_shared<Chunk>(chunkAllocator, chunkIndex);

  BlockArrayBox<block::Type> composition = terrain::generateNew(chunkIndex);
  BlockArrayBox<block::Light> lighting = calculateLighting(composition);
  chunk->setComposition(std::move(composition));
  chunk->setLighting(std::move(lighting));

  bool insertionSuccess = m_ChunkContainer.insert(chunkIndex, std::move(chunk));
  if (insertionSuccess)
    for (const GlobalIndex& stencilIndex : Chunk::Stencil(chunkIndex))
      addToLazyMeshUpdateQueue(stencilIndex);
  return chunk;
}

void ChunkManager::eraseChunk(const GlobalIndex& chunkIndex)
{
  if (!isInRange(chunkIndex, player::originIndex(), param::UnloadDistance()))
    m_ChunkContainer.erase(chunkIndex);
}

void ChunkManager::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  for (const LocalIndex& localIndex : affectedChunks(blockIndex))
  {
    GlobalIndex neighborIndex = chunkIndex + localIndex.upcast<globalIndex_t>();
    localIndex.l1Norm() > 1 ? addToLazyMeshUpdateQueue(neighborIndex) : addToForceMeshUpdateQueue(neighborIndex);
  }
}



void ChunkManager::meshChunk(const Chunk& chunk)
{
  const GlobalIndex& chunkIndex = chunk.globalIndex();

  // Return early if chunk is entirely air
  if (!chunk.composition())
  {
    removeMeshes(chunkIndex);
    return;
  }
  ENG_PROFILE_FUNCTION();

  BlockData blockData;
  blockData.composition = m_ChunkContainer.retrieveTypeData(chunk, { BlockData::Bounds() });
  blockData.lighting = m_ChunkContainer.retrieveLightingData(chunk, { BlockData::Bounds() });

  ChunkDrawCommand opaqueDraw(chunkIndex, false);
  ChunkDrawCommand transparentDraw(chunkIndex, true);
  for (const BlockIndex& blockIndex : Chunk::Bounds())
  {
    block::Type blockType = blockData.composition(blockIndex);

    if (blockType == block::ID::Air)
      continue;

    eng::EnumBitMask<eng::math::Direction> enabledFaces;
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
        for (const BlockIndex& lightIndex : lightingStencil)
        {
          if (!blockData.composition(lightIndex).hasTransparency())
            continue;

          totalSunlight += blockData.lighting(lightIndex).sunlight();
          transparentNeighbors++;
        }

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
  }

  m_OpaqueMultiDrawArray->queueCommand(std::move(opaqueDraw));
  m_TransparentMultiDrawArray->queueCommand(std::move(transparentDraw));
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
  for (const BlockIndex& blockIndex : BlockData::Bounds().faceInterior(eng::math::Direction::Top))
  {
    BlockIndex propogationIndex = blockIndex;
    if (blockData.lighting(propogationIndex) != block::Light::MaxValue())
      continue;

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
  }

  // Light unlit blocks neighboring sunlight with attenuated sunlight value and add them to the propogation stack
  std::array<std::stack<BlockIndex>, block::Light::MaxValue() + 1> sunlight;
  attenuatedSunlightExtents.forEach([&blockData, &sunlight](const BlockIndex2D& index, blockIndex_t k)
  {
    static constexpr i8 attenuatedIntensity = block::Light::MaxValue() - attenuation;
    for (BlockIndex blockIndex(index, k); blockIndex.k < Chunk::Size(); ++blockIndex.k)
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

    for (const BlockIndex& blockIndex : BlockData::Bounds().faceInterior(direction))
      if (blockData.composition(blockIndex).hasTransparency())
        sunlight[blockData.lighting(blockIndex).sunlight()].push(blockIndex);
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
        for (const LocalIndex& localIndex : affectedChunks(chunkSection))
          additionalLightingUpdates.insert(chunkIndex + localIndex.upcast<globalIndex_t>());
  });

  chunk.setLighting(std::move(newLighting));

  additionalLightingUpdates.erase(chunkIndex);
  for (const GlobalIndex& updateIndex : additionalLightingUpdates)
    addToLightingUpdateQueue(updateIndex);

  addToLazyMeshUpdateQueue(chunkIndex);
}

void ChunkManager::lightingTask(const GlobalIndex& chunkIndex)
{
  if (m_ChunkContainer.hasBoundaryNeighbors(chunkIndex))
    return;

  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    return;

  updateLighting(*chunk);
}

void ChunkManager::lazyMeshingTask(const GlobalIndex& chunkIndex)
{
  if (m_ChunkContainer.hasBoundaryNeighbors(chunkIndex))
    return;

  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    return;

  meshChunk(*chunk);
  chunk->update();
}

void ChunkManager::forceMeshingTask(const GlobalIndex& chunkIndex)
{
  std::shared_ptr<Chunk> chunk = m_ChunkContainer.chunks().get(chunkIndex);
  if (!chunk)
    chunk = generateNewChunk(chunkIndex);

  meshChunk(*chunk);
  chunk->update();
}