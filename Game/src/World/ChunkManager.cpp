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
    s_Shader = Engine::Shader::Create("assets/shaders/Voxel.glsl");
    s_Uniform = Engine::Uniform::Create(c_UniformBinding, sizeof(UniformData));
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

  Vec3 playerPosition = Player::Position();
  GlobalIndex originIndex = Player::OriginIndex();

  s_Shader->bind();
  s_Uniform->bind();
  s_TextureArray->bind(c_TextureSlot);

  {
    Engine::RenderCommand::SetBlending(false);
    Engine::RenderCommand::SetFaceCulling(true);
    Engine::RenderCommand::SetDepthWriting(true);
    Engine::RenderCommand::SetUseDepthOffset(false);
    UniformData uniformData = { .transparencyPass = false };
    s_Uniform->set(&uniformData, sizeof(UniformData));

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
    Engine::RenderCommand::MultiDrawVertices(drawCommands.data(), commandCount, sizeof(Chunk::DrawCommand));
  }

  {
    Engine::RenderCommand::SetBlending(true);
    Engine::RenderCommand::SetFaceCulling(false);
    Engine::RenderCommand::SetDepthWriting(false);
    Engine::RenderCommand::SetUseDepthOffset(true);
    Engine::RenderCommand::SetDepthOffset(-1.0f, -1.0f);
    UniformData uniformData = { .transparencyPass = true };
    s_Uniform->set(&uniformData, sizeof(UniformData));

    int commandCount = m_TransparentMultiDrawArray->mask([&originIndex, &frustumPlanes](const GlobalIndex& chunkIndex)
      {
        Vec3 anchorPosition = Chunk::AnchorPosition(chunkIndex, originIndex);
        Vec3 chunkCenter = Chunk::Center(anchorPosition);
        return Util::IsInRange(chunkIndex, originIndex, c_RenderDistance) && Util::IsInFrustum(chunkCenter, frustumPlanes);
      });
    m_TransparentMultiDrawArray->sort(commandCount, [&originIndex, &playerPosition](const GlobalIndex& chunkA, const GlobalIndex& chunkB)
      {
        // NOTE: Maybe measure min distance to chunk faces instead

        Vec3 anchorA = Chunk::AnchorPosition(chunkA, originIndex);
        Vec3 centerA = Chunk::Center(anchorA);
        length_t distA = glm::length2(centerA - playerPosition);

        Vec3 anchorB = Chunk::AnchorPosition(chunkB, originIndex);
        Vec3 centerB = Chunk::Center(anchorB);
        length_t distB = glm::length2(centerB - playerPosition);

        return distA > distB;
      });
    m_TransparentMultiDrawArray->amend(commandCount, [&originIndex, &playerPosition](Chunk::DrawCommand& drawCommand)
      {
        drawCommand.sortVoxels(originIndex, playerPosition);
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
      return !Util::IsInRange(chunk.getGlobalIndex(), originIndex, c_UnloadDistance);
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
      EN_WARN("Cannot call placeBlock block with no collision!");
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

void ChunkManager::loadChunk(const GlobalIndex& chunkIndex, Block::Type blockType)
{
  Array3D<Block::Type, Chunk::Size()> composition = AllocateArray3D<Block::Type, Chunk::Size()>(blockType);
  m_ChunkContainer.insert(chunkIndex, std::move(composition));
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
  Array3D<Block::Type, Chunk::Size()> composition;
  switch (m_LoadMode)
  {
    case ChunkManager::NotSet:  EN_ERROR("Load mode not set!");                     break;
    case ChunkManager::Void:    composition = Terrain::GenerateEmpty(chunkIndex);   break;
    case ChunkManager::Terrain: composition = Terrain::GenerateNew(chunkIndex);     break;
    default:                    EN_ERROR("Unknown load mode!");
  }

  m_ChunkContainer.insert(chunkIndex, std::move(composition));
}

static Block::Type getBlockType(const Array3D<Block::Type, Chunk::Size() + 2>& blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k)
{
  return blockData[i + 1][j + 1][k + 1];
}

static Block::Type getBlockType(const Array3D<Block::Type, Chunk::Size() + 2>& blockData, const BlockIndex& blockIndex)
{
  return getBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k);
}

static void setBlockType(Array3D<Block::Type, Chunk::Size() + 2>& blockData, blockIndex_t i, blockIndex_t j, blockIndex_t k, Block::Type blockType)
{
  blockData[i + 1][j + 1][k + 1] = blockType;
}

static void setBlockType(Array3D<Block::Type, Chunk::Size() + 2>& blockData, const BlockIndex& blockIndex, Block::Type blockType)
{
  setBlockType(blockData, blockIndex.i, blockIndex.j, blockIndex.k, blockType);
}

const Array3D<Block::Type, Chunk::Size() + 2>& ChunkManager::getBlockData(const GlobalIndex& chunkIndex) const
{
  thread_local Array3D<Block::Type, Chunk::Size() + 2> blockData = AllocateArray3D<Block::Type, Chunk::Size() + 2>();

  blockData.fill(Block::Type::Null);

  // Load blocks from chunk
  {
    auto [chunk, lock] = m_ChunkContainer.acquireChunk(chunkIndex);
    if (!chunk || chunk->empty())
      return blockData;

    for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
      for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
        for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
          setBlockType(blockData, i, j, k, chunk->getBlockType(i, j, k));
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
            setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
          }
          else
            setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
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

            setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
          }
          else
            setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
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
            setBlockType(blockData, relativeBlockIndex, neighbor->getBlockType(neighborBlockIndex));
          }
          else
            setBlockType(blockData, relativeBlockIndex, Block::Type::Air);
        }
      }

  return blockData;
}

bool ChunkManager::meshChunk(const GlobalIndex& chunkIndex)
{
  const Array3D<Block::Type, Chunk::Size() + 2>& blockData = getBlockData(chunkIndex);
  if (getBlockType(blockData, 0, 0, 0) == Block::Type::Null)
  {
    m_OpaqueCommandQueue.emplace(chunkIndex);
    m_TransparentCommandQueue.emplace(chunkIndex);
    return false;
  }

  EN_PROFILE_FUNCTION();

  std::vector<Chunk::Voxel> opaqueMesh;
  std::vector<Chunk::Voxel> transparentMesh;
  for (blockIndex_t i = 0; i < Chunk::Size(); ++i)
    for (blockIndex_t j = 0; j < Chunk::Size(); ++j)
      for (blockIndex_t k = 0; k < Chunk::Size(); ++k)
      {
        BlockIndex blockIndex = BlockIndex(i, j, k);
        Block::Type blockType = getBlockType(blockData, blockIndex);

        if (blockType == Block::Type::Air)
          continue;

        uint32_t adjacencyData = 0;
        for (Direction direction : Directions())
        {
          BlockIndex offset = BlockIndex::Dir(direction);
          Block::Type blockNeighbor = getBlockType(blockData, blockIndex + offset);
          if (blockNeighbor != blockType && (Block::HasTransparency(blockType) || Block::HasTransparency(blockNeighbor)))
            adjacencyData |= 1 << static_cast<int>(direction);
        }

        if (!adjacencyData)
          continue;

        uint32_t voxelData = static_cast<blockID>(blockType);
        voxelData |= blockIndex.i << 16;
        voxelData |= blockIndex.j << 21;
        voxelData |= blockIndex.k << 26;
        if (Block::HasTransparency(blockType))
          voxelData |= 1 << 31;

        for (auto itA = Directions().begin(); itA != Directions().end(); ++itA)
          for (auto itB = itA.next(); itB != Directions().end(); ++itB)
          {
            Direction faceA = *itA;
            Direction faceB = *itB;

            // Opposite faces cannot form edge
            if (faceB == !faceA)
              continue;

            BlockIndex blockOffset = BlockIndex::Dir(faceA) + BlockIndex::Dir(faceB);
            Block::Type edgeNeighbor = getBlockType(blockData, blockIndex + blockOffset);

            if (!Block::HasTransparency(edgeNeighbor))
            {
              int adjacencyIndex = 9 * (blockOffset.i + 1) + 3 * (blockOffset.j + 1) + (blockOffset.k + 1);
              int edgeIndex = (adjacencyIndex - 1) / 2;
              if (edgeIndex > 5)
                edgeIndex--;
              adjacencyData |= 1 << (edgeIndex + 6);
            }
          }

        for (int I = 0; I < 2; ++I)
          for (int J = 0; J < 2; ++J)
            for (int K = 0; K < 2; ++K)
            {
              BlockIndex blockOffset(2 * I - 1, 2 * J - 1, 2 * K - 1);
              Block::Type cornerNeighbor = getBlockType(blockData, blockIndex + blockOffset);

              if (!Block::HasTransparency(cornerNeighbor))
              {
                int cornerIndex = 4 * I + 2 * J + K;
                adjacencyData |= 1 << (cornerIndex + 18);
              }
            }

        std::vector<Chunk::Voxel>& mesh = Block::HasTransparency(blockType) ? transparentMesh : opaqueMesh;
        mesh.push_back(Chunk::Voxel(voxelData, adjacencyData));
      }

  bool meshGenrated = !opaqueMesh.empty() || !transparentMesh.empty();
  m_OpaqueCommandQueue.emplace(chunkIndex, std::move(opaqueMesh));
  m_TransparentCommandQueue.emplace(chunkIndex, std::move(transparentMesh));
  return meshGenrated;
}