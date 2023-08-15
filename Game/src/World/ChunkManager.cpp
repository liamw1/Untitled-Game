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
  if (m_MeshingThread.joinable())
    m_MeshingThread.join();
  if (m_LightingThread.joinable())
    m_LightingThread.join();
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

  m_OpaqueMultiDrawArray = std::make_unique<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>(s_VertexBufferLayout);
  m_TransparentMultiDrawArray = std::make_unique<Engine::MultiDrawIndexedArray<Chunk::DrawCommand>>(s_VertexBufferLayout);

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

void ChunkManager::launchMeshingThread()
{
  m_MeshingThread = std::thread(&ChunkManager::meshingWorker, this);
}

void ChunkManager::launchLightingThread()
{
  m_LightingThread = std::thread(&ChunkManager::lightingWorker, this);
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

void ChunkManager::meshingWorker()
{
  using namespace std::chrono_literals;

  while (m_Running.load())
  {
    EN_PROFILE_SCOPE("Meshing worker");

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

void ChunkManager::lightingWorker()
{
  using namespace std::chrono_literals;

  while (m_Running.load())
  {
    EN_PROFILE_SCOPE("Lighting worker");

    std::optional<GlobalIndex> updateIndex = m_LightingUpdateQueue.tryRemove();
    if (!updateIndex)
    {
      std::this_thread::sleep_for(100ms);
      continue;
    }
    if (m_ChunkContainer.hasBoundaryNeighbors(*updateIndex))
      continue;

    updateLighting(*updateIndex);
  }
}

void ChunkManager::addToLightingUpdateQueue(const GlobalIndex& chunkIndex)
{
  m_LightingUpdateQueue.add(chunkIndex);
}

void ChunkManager::addToLazyMeshUpdateQueue(const GlobalIndex& chunkIndex)
{
  if (!m_ForceMeshUpdateQueue.contains(chunkIndex) && !m_LightingUpdateQueue.contains(chunkIndex))
    m_LazyMeshUpdateQueue.add(chunkIndex);
}

void ChunkManager::addToForceMeshUpdateQueue(const GlobalIndex& chunkIndex)
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
          addToLazyMeshUpdateQueue(chunkIndex + GlobalIndex(i, j, k));
}

void ChunkManager::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
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



ChunkManager::BlockData::BlockData()
  : composition(MakeCubicArray<Block::Type, Chunk::Size() + 2, -1>()),
    lighting(MakeCubicArray<Block::Light, Chunk::Size() + 2, -1>()) {}

void ChunkManager::BlockData::fill(const BlockBox& fillSection, const Chunk* chunk, const BlockIndex& chunkBase, bool fillLight)
{
  if (chunk->composition())
    composition.fill(fillSection, chunk->composition(), chunkBase);
  else
    composition.fill(fillSection, Block::Type::Air);

  if (!fillLight)
    return;

  if (chunk->lighting())
    lighting.fill(fillSection, chunk->lighting(), chunkBase);
  else
    lighting.fill(fillSection, Block::Light::MaxValue());
}



template<typename T, int Len, int Base>
static BlockBox faceInterior(const CubicArray<T, Len, Base>& arr, Direction direction)
{
  static constexpr blockIndex_t limits[2] = { Base, Len + Base - 1 };

  blockIndex_t faceLimit = limits[IsUpstream(direction)];
  BlockIndex fillLower = BlockIndex::CreatePermuted(faceLimit, 0, 0, GetCoordID(direction));
  BlockIndex fillUpper = BlockIndex::CreatePermuted(faceLimit + 1, Chunk::Size(), Chunk::Size(), GetCoordID(direction));

  return { fillLower, fillUpper };
}

template<typename T, int Len, int Base>
static BlockBox edgeInterior(const CubicArray<T, Len, Base>& arr, Direction faceA, Direction faceB)
{
  EN_ASSERT(faceA != faceB, "Opposite faces cannot form an edge!");

  static constexpr blockIndex_t limits[2] = { Base, Len + Base - 1 };

  int u = GetCoordID(faceA);
  int v = GetCoordID(faceB);
  int w = (2 * (u + v)) % 3;  // Extracts coordID that runs along edge

  BlockIndex fillLower(0);
  fillLower[u] = limits[IsUpstream(faceA)];
  fillLower[v] = limits[IsUpstream(faceB)];
  BlockIndex fillUpper(Chunk::Size());
  fillUpper[u] = fillLower[u] + 1;
  fillUpper[v] = fillLower[v] + 1;

  return { fillLower, fillUpper };
}

template<typename T, int Len, int Base>
static BlockBox corner(const CubicArray<T, Len, Base>& arr, const GlobalIndex& offset)
{
  static constexpr blockIndex_t limits[2] = { Base, Len + Base - 1 };

  BlockIndex fillIndex = { limits[offset.i > 0], limits[offset.j > 0], limits[offset.k > 0] };
  return { fillIndex, fillIndex + 1 };
}



ChunkManager::BlockData& ChunkManager::getBlockData(const GlobalIndex& chunkIndex, bool getInteriorLighting) const
{
  EN_PROFILE_FUNCTION();

  thread_local BlockData blockData;
  blockData.composition.fill(Block::Type::Null);
  blockData.lighting.fill(Block::Light());

  // Load blocks from chunk
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    if (!chunk)
      return blockData;

    BlockBox fillSection(0, Chunk::Size());
    blockData.fill(fillSection, chunk, BlockIndex(0), getInteriorLighting);
  }

  // Load blocks from cardinal neighbors
  for (Direction direction : Directions())
  {
    auto [neighbor, lock] = m_ChunkContainer.acquireChunk(chunkIndex + GlobalIndex::Dir(direction));
    if (!neighbor)
      continue;

    BlockBox fillSection = faceInterior(blockData.composition, direction);
    BlockIndex neighborBase = faceInterior(neighbor->composition(), !direction).min;

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

      BlockBox fillSection = edgeInterior(blockData.composition, faceA, faceB);
      BlockIndex neighborBase = edgeInterior(neighbor->composition(), !faceA, !faceB).min;

      blockData.fill(fillSection, neighbor, neighborBase);
    }

  // Load blocks from corner neighbors
  for (int i = -1; i <= 1; i += 2)
    for (int j = -1; j <= 1; j += 2)
      for (int k = -1; k <= 1; k += 2)
      {
        GlobalIndex offset(i, j, k);
        GlobalIndex neighborIndex = chunkIndex + offset;

        auto [neighbor, lock] = m_ChunkContainer.acquireChunk(neighborIndex);
        if (!neighbor)
          continue;

        BlockBox fillSection = corner(blockData.composition, offset);
        BlockIndex neighborCorner = corner(neighbor->composition(), -offset).min;

        blockData.fill(fillSection, neighbor, neighborCorner);
      }

  return blockData;
}

void ChunkManager::meshChunk(const GlobalIndex& chunkIndex)
{
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    if (!chunk || !chunk->composition())
    {
      m_OpaqueCommandQueue.emplace(chunkIndex, false);
      m_TransparentCommandQueue.emplace(chunkIndex, false);
      return;
    }
  }

  EN_PROFILE_FUNCTION();

  const BlockData& blockData = getBlockData(chunkIndex);
  if (blockData.composition(BlockIndex(0)) == Block::Type::Null)
    return;

  Chunk::DrawCommand opaqueDraw(chunkIndex, true);
  Chunk::DrawCommand transparentDraw(chunkIndex, false);
  Chunk::Bounds().forEach([&blockData, &opaqueDraw, &transparentDraw](const BlockIndex& blockIndex)
    {
      Block::Type blockType = blockData.composition(blockIndex);

      if (blockType == Block::Type::Air)
        return;

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

      if (enabledFaces != 0)
        draw.addVoxel(blockIndex, enabledFaces);
    });
  opaqueDraw.setIndices();

  m_OpaqueCommandQueue.add(std::move(opaqueDraw));
  m_TransparentCommandQueue.add(std::move(transparentDraw));
}

void ChunkManager::updateLighting(const GlobalIndex& chunkIndex)
{
  BlockData& blockData = getBlockData(chunkIndex, false);
  if (blockData.composition(BlockIndex(0)) == Block::Type::Null)
    return;

  EN_PROFILE_FUNCTION();

  std::array<std::stack<BlockIndex>, Block::Light::MaxValue() + 1> light;
  BlockData::Bounds().forEach([&blockData, &light](const BlockIndex& blockIndex)
    {
      if (Chunk::Bounds().encloses(blockIndex) || !Block::HasTransparency(blockData.composition(blockIndex)))
        return;

      light[blockData.lighting(blockIndex).sunlight()].push(blockIndex);
    });

  for (int8_t intensity = Block::Light::MaxValue(); intensity > 0; --intensity)
    while (!light[intensity].empty())
    {
      BlockIndex lightIndex = light[intensity].top();
      light[intensity].pop();

      for (Direction direction : Directions())
      {
        BlockIndex lightNeighbor = lightIndex + BlockIndex::Dir(direction);
        if (!Chunk::Bounds().encloses(lightNeighbor) || !Block::HasTransparency(blockData.composition(lightNeighbor)))
          continue;

        int8_t neighborIntensity = intensity == Block::Light::MaxValue() && direction == Direction::Bottom ? intensity : intensity - 1;
        if (neighborIntensity <= blockData.lighting(lightNeighbor).sunlight())
          continue;

        blockData.lighting(lightNeighbor) = neighborIntensity;
        light[neighborIntensity].push(lightNeighbor);
      }
    }

  CubicArray<Block::Light, Chunk::Size()> newLighting;
  if (!blockData.lighting.filledWith(Chunk::Bounds(), Block::Light::MaxValue()))
  {
    newLighting = MakeCubicArray<Block::Light, Chunk::Size()>();
    newLighting.fill(Chunk::Bounds(), blockData.lighting, BlockIndex(0));
  }

  auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
  if (!chunk)
    return;

  std::unordered_set<GlobalIndex> additionalLightingUpdates;

  for (Direction direction : Directions())
  {
    BlockBox compareSection = faceInterior(chunk->lighting(), direction);
    if (!newLighting.contentsEqual(compareSection, chunk->lighting(), compareSection.min))
      additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(direction));
  }
  
  for (auto itA = Directions().begin(); itA != Directions().end(); ++itA)
    for (auto itB = itA.next(); itB != Directions().end(); ++itB)
    {
      Direction faceA = *itA;
      Direction faceB = *itB;
  
      // Opposite faces cannot form edge
      if (faceB == !faceA)
        continue;
  
      BlockBox compareSection = edgeInterior(chunk->lighting(), faceA, faceB);
      if (!newLighting.contentsEqual(compareSection, chunk->lighting(), compareSection.min))
      {
        additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(faceA));
        additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(faceB));
        additionalLightingUpdates.insert(chunkIndex + GlobalIndex::Dir(faceA) + GlobalIndex::Dir(faceB));
      }
    }
  
  for (int i = -1; i < 2; i += 2)
    for (int j = -1; j < 2; j += 2)
      for (int k = -1; k < 2; k += 2)
      {
        GlobalIndex offset(i, j, k);
  
        BlockBox compareSection = corner(chunk->lighting(), offset);
        if (!newLighting.contentsEqual(compareSection, chunk->lighting(), compareSection.min))
        {
          GlobalIndex offsetI{}, offsetJ{}, offsetK{};
          offsetI.i = offset.i;
          offsetJ.j = offset.j;
          offsetK.k = offset.k;
  
          additionalLightingUpdates.insert(chunkIndex + offsetI);
          additionalLightingUpdates.insert(chunkIndex + offsetJ);
          additionalLightingUpdates.insert(chunkIndex + offsetK);
          additionalLightingUpdates.insert(chunkIndex + offsetI + offsetJ);
          additionalLightingUpdates.insert(chunkIndex + offsetI + offsetK);
          additionalLightingUpdates.insert(chunkIndex + offsetJ + offsetK);
          additionalLightingUpdates.insert(chunkIndex + offset);
        }
      }

  chunk->setLighting(std::move(newLighting));
  addToLazyMeshUpdateQueue(chunkIndex);
  for (const GlobalIndex& updateIndex : additionalLightingUpdates)
    addToLightingUpdateQueue(updateIndex);
}