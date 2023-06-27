#include "GMpch.h"
#include "ChunkContainer.h"
#include "Util/Util.h"
#include "Player/Player.h"

static constexpr int c_MaxChunks = (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1) * (2 * c_UnloadDistance + 1);

void ChunkContainer::initialize()
{
  if (!s_IndexBuffer)
  {
    constexpr uint32_t maxIndices = 6 * 6 * Chunk::TotalBlocks();

    uint32_t offset = 0;
    uint32_t* indices = new uint32_t[maxIndices];
    for (uint32_t i = 0; i < maxIndices; i += 6)
    {
      // Triangle 1
      indices[i + 0] = offset + 0;
      indices[i + 1] = offset + 1;
      indices[i + 2] = offset + 2;

      // Triangle 2
      indices[i + 3] = offset + 2;
      indices[i + 4] = offset + 3;
      indices[i + 5] = offset + 0;

      offset += 4;
    }
    s_IndexBuffer = Engine::IndexBuffer::Create(indices, maxIndices);
    delete[] indices;

    s_Shader = Engine::Shader::Create("assets/shaders/Chunk.glsl");
    s_TextureArray = Block::GetTextureArray();
    Engine::UniformBuffer::Allocate(c_UniformBinding, 1024 * sizeof(Float4));
  }

  m_MultiDrawArray = std::make_unique<Engine::MultiArray<GlobalIndex>>(s_VertexBufferLayout);
  m_MultiDrawArray->setIndexBuffer(s_IndexBuffer);
  m_ChunkArray = std::make_unique<Chunk[]>(c_MaxChunks);

  std::vector<int> stackData;
  stackData.reserve(c_MaxChunks);
  m_OpenChunkSlots = std::stack<int, std::vector<int>>(std::move(stackData));
  for (int i = c_MaxChunks - 1; i >= 0; --i)
    m_OpenChunkSlots.push(i);
}

void ChunkContainer::render()
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

  if (!m_MeshUpdateQueue.empty())
  {
    std::shared_lock lock(m_ContainerMutex);
    uploadMeshes();
  }

  s_Shader->bind();
  s_TextureArray->bind(c_TextureSlot);
  m_MultiDrawArray->bind();
  Engine::UniformBuffer::Bind(c_UniformBinding);

  GlobalIndex originIndex = Player::OriginIndex();
  int commandCount = m_MultiDrawArray->mask([&originIndex, &frustumPlanes](const GlobalIndex& chunkIndex)
    {
      Vec3 anchorPosition = Chunk::AnchorPosition(chunkIndex, originIndex);
      Vec3 chunkCenter = Chunk::Center(anchorPosition);
      return Util::IsInFrustum(chunkCenter, frustumPlanes) && Util::IsInRange(chunkIndex, originIndex, c_RenderDistance);
    });

  const std::vector<Engine::MultiArray<GlobalIndex>::DrawCommand>& drawCommands = m_MultiDrawArray->getDrawCommandBuffer();
  for (int batchStart = 0; batchStart < commandCount; batchStart += 1024)
  {
    int batchEnd = std::min(batchStart + 1024, commandCount);
    int batchSize = batchEnd - batchStart;

    std::vector<Float4> uniforms;
    uniforms.reserve(batchSize);

    for (int i = 0; i < batchSize; ++i)
    {
      const GlobalIndex& chunkIndex = drawCommands[batchStart + i].ID;
      Vec3 chunkAnchor = Chunk::AnchorPosition(chunkIndex, originIndex);
      uniforms.emplace_back(chunkAnchor, 0);
    }

    Engine::UniformBuffer::SetData(c_UniformBinding, uniforms.data(), static_cast<uint32_t>(uniforms.size() * sizeof(Float4)));
    Engine::RenderCommand::MultiDrawIndexed(drawCommands.data() + batchStart, batchSize, sizeof(Engine::MultiArray<GlobalIndex>::DrawCommand));
  }
}

bool ChunkContainer::insert(const GlobalIndex& chunkIndex, Array3D<Block::Type, Chunk::Size()> chunkComposition)
{
  std::lock_guard lock(m_ContainerMutex);

  if (m_OpenChunkSlots.empty())
    return false;

  // Grab first open chunk slot
  int chunkSlot = m_OpenChunkSlots.top();
  m_OpenChunkSlots.pop();

  // Insert chunk into array and load it
  Chunk& chunk = m_ChunkArray[chunkSlot];
  chunk = Chunk(chunkIndex);
  chunk.setData(std::move(chunkComposition));
  auto [insertionPosition, insertionSuccess] = m_BoundaryChunks.insert({ chunk.getGlobalIndex(), &chunk });

  if (insertionSuccess)
    sendChunkLoadUpdate(chunk);

  return insertionSuccess;
}

bool ChunkContainer::erase(const GlobalIndex& chunkIndex)
{
  std::lock_guard lock(m_ContainerMutex);

  mapType<GlobalIndex, Chunk*>::iterator erasePosition = m_BoundaryChunks.find(chunkIndex);
  if (erasePosition == m_BoundaryChunks.end())
    return false;
  const Chunk* chunk = erasePosition->second;

  // Open up chunk slot
  int chunkSlot = static_cast<int>(chunk - &m_ChunkArray[0]);
  m_OpenChunkSlots.push(chunkSlot);

  m_MultiDrawArray->remove(chunkIndex);

  // Delete chunk data
  {
    std::lock_guard chunkLock = chunk->acquireLock();
    m_ChunkArray[chunkSlot].reset();
  }
  m_BoundaryChunks.erase(erasePosition);

  sendChunkRemovalUpdate(chunkIndex);

  return true;
}

bool ChunkContainer::update(const GlobalIndex& chunkIndex, std::vector<uint32_t>&& mesh)
{
  std::shared_lock sharedLock(m_ContainerMutex);

  Chunk* chunk = find(chunkIndex);
  if (!chunk)
    return false;

  ChunkType source = getChunkType(*chunk);
  if (source == ChunkType::Boundary)
    return false;

  ChunkType destination;
  {
    std::lock_guard chunkLock = chunk->acquireLock();
    sharedLock.unlock();

    bool madeEmpty = !chunk->empty() && mesh.empty() && chunk->getBlockType(0, 0, 0) == Block::Type::Air;

    chunk->update(madeEmpty);
    m_MeshUpdateQueue.add(chunkIndex, std::move(mesh));

    destination = chunk->empty() ? ChunkType::Empty : ChunkType::Renderable;
  }

  if (source != destination)
  {
    std::lock_guard lock(m_ContainerMutex);
    recategorizeChunk(*chunk, source, destination);
  }

  return true;
}

std::unordered_set<GlobalIndex> ChunkContainer::findAllLoadableIndices() const
{
  std::shared_lock lock(m_ContainerMutex);

  GlobalIndex originIndex = Player::OriginIndex();
  std::unordered_set<GlobalIndex> newChunkIndices;
  for (const auto& [key, chunk] : m_BoundaryChunks)
    for (Direction direction : Directions())
    {
      GlobalIndex neighborIndex = chunk->getGlobalIndex() + GlobalIndex::Dir(direction);
      if (Util::IsInRange(neighborIndex, originIndex, c_LoadDistance) && newChunkIndices.find(neighborIndex) == newChunkIndices.end())
        if (!chunk->isFaceOpaque(direction) && !isLoaded(neighborIndex))
          newChunkIndices.insert(neighborIndex);
    }
  return newChunkIndices;
}

std::pair<Chunk*, std::unique_lock<std::mutex>> ChunkContainer::acquireChunk(const GlobalIndex& chunkIndex)
{
  std::shared_lock lock(m_ContainerMutex);
  std::unique_lock<std::mutex> chunkLock;

  Chunk* chunk = find(chunkIndex);
  if (chunk)
    chunkLock = std::unique_lock(chunk->m_Mutex);

  return { chunk, std::move(chunkLock) };
}

std::pair<const Chunk*, std::unique_lock<std::mutex>> ChunkContainer::acquireChunk(const GlobalIndex& chunkIndex) const
{
  std::shared_lock lock(m_ContainerMutex);
  std::unique_lock<std::mutex> chunkLock;

  const Chunk* chunk = find(chunkIndex);
  if (chunk)
    chunkLock = std::unique_lock(chunk->m_Mutex);

  return { chunk, std::move(chunkLock) };
}

void ChunkContainer::sendBlockUpdate(const GlobalIndex& chunkIndex, const BlockIndex& blockIndex)
{
  m_ForceUpdateQueue.add(chunkIndex);

  std::vector<Direction> updateDirections{};
  for (Direction direction : Directions())
    if (Util::BlockNeighborIsInAnotherChunk(blockIndex, direction))
      updateDirections.push_back(direction);

  EN_ASSERT(updateDirections.size() <= 3, "Too many update directions for a single block update!");

  // Update neighbors in cardinal directions
  for (Direction direction : updateDirections)
  {
    GlobalIndex neighborIndex = chunkIndex + GlobalIndex::Dir(direction);
    m_ForceUpdateQueue.add(neighborIndex);
  }

  // Queue edge neighbor for updating
  if (updateDirections.size() == 2)
  {
    GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::Dir(updateDirections[0]) + GlobalIndex::Dir(updateDirections[1]);
    m_LazyUpdateQueue.add(edgeNeighborIndex);
  }

  // Queue edge/corner neighbors for updating
  if (updateDirections.size() == 3)
  {
    GlobalIndex cornerNeighborIndex = chunkIndex;
    for (int i = 0; i < 3; ++i)
    {
      int j = (i + 1) % 3;
      GlobalIndex edgeNeighborIndex = chunkIndex + GlobalIndex::Dir(updateDirections[i]) + GlobalIndex::Dir(updateDirections[j]);
      m_LazyUpdateQueue.add(edgeNeighborIndex);

      cornerNeighborIndex += GlobalIndex::Dir(updateDirections[i]);
    }

    m_LazyUpdateQueue.add(cornerNeighborIndex);
  }
}

bool ChunkContainer::empty() const
{
  std::shared_lock lock(m_ContainerMutex);
  return m_OpenChunkSlots.size() == c_MaxChunks;
}

bool ChunkContainer::contains(const GlobalIndex& chunkIndex) const
{
  std::shared_lock lock(m_ContainerMutex);
  return isLoaded(chunkIndex);
}



bool ChunkContainer::IndexSet::add(const GlobalIndex& index)
{
  std::lock_guard lock(m_Mutex);

  auto [insertionPosition, insertionSuccess] = m_Data.insert(index);
  return insertionSuccess;
}

std::optional<GlobalIndex> ChunkContainer::IndexSet::tryRemove()
{
  std::lock_guard lock(m_Mutex);

  if (!m_Data.empty())
  {
    GlobalIndex value = *m_Data.begin();
    m_Data.erase(m_Data.begin());
    return value;
  }
  return std::nullopt;
}



void ChunkContainer::MeshMap::add(const GlobalIndex& index, std::vector<uint32_t>&& mesh)
{
  std::lock_guard lock(m_Mutex);
  m_Data[index] = std::move(mesh);
}

std::optional<std::pair<GlobalIndex, std::vector<uint32_t>>> ChunkContainer::MeshMap::tryRemove()
{
  std::lock_guard lock(m_Mutex);

  if (!m_Data.empty())
  {
    std::pair<GlobalIndex, std::vector<uint32_t>> keyVal = std::move(*m_Data.begin());
    m_Data.erase(m_Data.begin());
    return keyVal;
  }
  return std::nullopt;
}

std::size_t ChunkContainer::MeshMap::empty()
{
  std::lock_guard lock(m_Mutex);
  return m_Data.empty();
}




bool ChunkContainer::isLoaded(const GlobalIndex& chunkIndex) const
{
  for (const mapType<GlobalIndex, Chunk*>& chunkGroup : m_Chunks)
    if (chunkGroup.find(chunkIndex) != chunkGroup.end())
      return true;
  return false;
}

bool ChunkContainer::isOnBoundary(const Chunk& chunk) const
{
  for (Direction direction : Directions())
    if (!isLoaded(chunk.getGlobalIndex() + GlobalIndex::Dir(direction)) && !chunk.isFaceOpaque(direction))
      return true;
  return false;
}

ChunkType ChunkContainer::getChunkType(const Chunk& chunk) const
{
  for (int chunkType = 0; chunkType < c_ChunkTypes; ++chunkType)
    if (m_Chunks[chunkType].find(chunk.getGlobalIndex()) != m_Chunks[chunkType].end())
      return static_cast<ChunkType>(chunkType);

  EN_ERROR("Could not find chunk!");
  return ChunkType::Error;
}

Chunk* ChunkContainer::find(const GlobalIndex& chunkIndex)
{
  for (mapType<GlobalIndex, Chunk*>& chunkGroup : m_Chunks)
  {
    mapType<GlobalIndex, Chunk*>::iterator it = chunkGroup.find(chunkIndex);

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

const Chunk* ChunkContainer::find(const GlobalIndex& chunkIndex) const
{
  for (const mapType<GlobalIndex, Chunk*>& chunkGroup : m_Chunks)
  {
    mapType<GlobalIndex, Chunk*>::const_iterator it = chunkGroup.find(chunkIndex);

    if (it != chunkGroup.end())
      return it->second;
  }
  return nullptr;
}

void ChunkContainer::sendChunkLoadUpdate(Chunk& newChunk)
{
  boundaryChunkUpdate(newChunk);

  // Move cardinal neighbors out of m_BoundaryChunks if they are no longer on boundary
  for (Direction direction : Directions())
  {
    Chunk* neighbor = find(newChunk.getGlobalIndex() + GlobalIndex::Dir(direction));
    if (neighbor && getChunkType(*neighbor) == ChunkType::Boundary)
      boundaryChunkUpdate(*neighbor);
  }
}

void ChunkContainer::sendChunkRemovalUpdate(const GlobalIndex& removalIndex)
{
  for (Direction direction : Directions())
  {
    Chunk* neighbor = find(removalIndex + GlobalIndex::Dir(direction));

    if (neighbor)
    {
      ChunkType type = getChunkType(*neighbor);

      if (type != ChunkType::Boundary)
        recategorizeChunk(*neighbor, type, ChunkType::Boundary);
    }
  }
}

void ChunkContainer::boundaryChunkUpdate(Chunk& chunk)
{
  EN_ASSERT(getChunkType(chunk) == ChunkType::Boundary, "Chunk is not a boundary chunk!");

  if (!isOnBoundary(chunk))
  {
    ChunkType destination;
    {
      std::lock_guard lock = chunk.acquireLock();
      destination = chunk.empty() ? ChunkType::Empty : ChunkType::Renderable;
    }
    recategorizeChunk(chunk, ChunkType::Boundary, destination);
  }
}

void ChunkContainer::recategorizeChunk(Chunk& chunk, ChunkType source, ChunkType destination)
{
  EN_ASSERT(source != destination, "Source and destination are the same!");
  EN_ASSERT(getChunkType(chunk) == source, "Chunk is not of the source type!");

  // Chunks moved from m_BoundaryChunks get queued for updating, along with all their neighbors
  if (source == ChunkType::Boundary)
    for (int i = -1; i <= 1; ++i)
      for (int j = -1; j <= 1; ++j)
        for (int k = -1; k <= 1; ++k)
        {
          GlobalIndex neighborIndex = chunk.getGlobalIndex() + GlobalIndex(i, j, k);
          if (isLoaded(neighborIndex))
            m_LazyUpdateQueue.add(neighborIndex);
        }

  int sourceTypeID = static_cast<int>(source);
  int destinationTypeID = static_cast<int>(destination);

  m_Chunks[destinationTypeID].insert({ chunk.getGlobalIndex(), &chunk});
  m_Chunks[sourceTypeID].erase(m_Chunks[sourceTypeID].find(chunk.getGlobalIndex()));
}

void ChunkContainer::uploadMeshes()
{
  EN_PROFILE_FUNCTION();

  std::optional<std::pair<GlobalIndex, std::vector<uint32_t>>> meshUpdate = m_MeshUpdateQueue.tryRemove();
  while (meshUpdate)
  {
    const auto& [updateIndex, mesh] = *meshUpdate;

    m_MultiDrawArray->remove(updateIndex);
    if (isLoaded(updateIndex))
    {
      uint32_t indexCount = static_cast<uint32_t>(3 * mesh.size() / 2);

      m_MultiDrawArray->add(updateIndex, mesh.data(), static_cast<int>(mesh.size()), indexCount);
    }

    meshUpdate = m_MeshUpdateQueue.tryRemove();
  }
}