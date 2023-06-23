#include "ENpch.h"
#include "MultiDrawArray.h"

namespace Engine
{
  static constexpr uint64_t c_BufferSize = bit(26);

  MultiDrawArray::MultiDrawArray(const BufferLayout& layout)
  {
    m_VertexArray = VertexArray::Create();
    m_VertexArray->setLayout(layout);
    m_Stride = layout.stride();

    std::unique_ptr<char[]> emptyBuffer = std::make_unique<char[]>(c_BufferSize);
    m_VertexArray->setVertexBuffer(emptyBuffer.get(), c_BufferSize);

    m_MemoryPool.push_back({ 0, c_BufferSize, true });
  }

  void MultiDrawArray::bind() const
  {
    m_VertexArray->bind();
  }

  void MultiDrawArray::unBind() const
  {
    m_VertexArray->unBind();
  }

  void MultiDrawArray::add(int ID, const void* data, uint32_t size)
  {
    if (size == 0)
      return;
    if (m_AllocatedMemory.find(ID) != m_AllocatedMemory.end())
    {
      EN_CORE_ERROR("Memory has already been allocated for region with ID {0}!", ID);
      return;
    }

    uint32_t minimumMemoryLeftover = std::numeric_limits<uint32_t>::max();
    std::vector<MemoryRegion>::iterator bestRegion = m_MemoryPool.begin();
    for (std::vector<MemoryRegion>::iterator memoryRegion = m_MemoryPool.begin(); memoryRegion != m_MemoryPool.end(); ++memoryRegion)
      if (memoryRegion->free)
      {
        uint32_t memoryLeftover = memoryRegion->size - size;  // Unsigned int wrap intended here if answer is negative
        if (memoryLeftover < minimumMemoryLeftover)
        {
          minimumMemoryLeftover = memoryLeftover;
          bestRegion = memoryRegion;
        }
      }

    if (minimumMemoryLeftover > c_BufferSize)
    {
      EN_CORE_ERROR("Could not find buffer region large enough to hold {0} bytes!", size);
      return;
    }

    m_VertexArray->modifyVertexBuffer(data, bestRegion->offset, size);
    m_AllocatedMemory.insert({ ID, bestRegion->offset });

    bestRegion->size = size;
    bestRegion->free = false;
    if (minimumMemoryLeftover > 0)
      m_MemoryPool.insert(std::next(bestRegion), { bestRegion->offset + size, minimumMemoryLeftover, true });
  }

  void MultiDrawArray::remove(int ID)
  {
    std::unordered_map<int, uint32_t>::iterator mapPosition = m_AllocatedMemory.find(ID);

    if (mapPosition == m_AllocatedMemory.end())
      return;

    uintptr_t allocationOffset = mapPosition->second;
    std::vector<MemoryRegion>::iterator freedRegion = std::find_if(m_MemoryPool.begin(), m_MemoryPool.end(), [=](const MemoryRegion& region) { return region.offset == allocationOffset; });

    if (freedRegion == m_MemoryPool.end())
    {
      EN_CORE_ERROR("Could not find buffer region associated with ID {0}!", ID);
      return;
    }
    freedRegion->free = true;

    if (freedRegion != m_MemoryPool.begin())
    {
      std::vector<MemoryRegion>::iterator prevRegion = std::prev(freedRegion);
      if (prevRegion->free)
      {
        freedRegion->offset = prevRegion->offset;
        freedRegion->size += prevRegion->size;
        freedRegion = m_MemoryPool.erase(prevRegion);
      }
    }

    std::vector<MemoryRegion>::iterator nextRegion = std::next(freedRegion);
    if (nextRegion != m_MemoryPool.end() && nextRegion->free)
    {
      freedRegion->size += nextRegion->size;
      m_MemoryPool.erase(nextRegion);
    }

    m_AllocatedMemory.erase(mapPosition);
  }

  uint32_t MultiDrawArray::getSize(int ID)
  {
    try
    {
      uint32_t allocationOffset = m_AllocatedMemory.at(ID);

      std::vector<MemoryRegion>::iterator freedRegion = std::find_if(m_MemoryPool.begin(), m_MemoryPool.end(), [=](const MemoryRegion& region) { return region.offset == allocationOffset; });
      return freedRegion->size;
    }
    catch (const std::out_of_range& /* e */)
    {
      EN_CORE_ERROR("Could not find buffer region assocatied with ID {0}!", ID);
      return 0;
    }
  }

  bool MultiDrawArray::queue(int ID, uint32_t indexCount)
  {
    try
    {
      uint32_t allocationOffset = m_AllocatedMemory.at(ID);

      DrawElementsIndirectCommand queuedDrawCommand{};
      queuedDrawCommand.count = indexCount == 0 ? m_VertexArray->getIndexBuffer()->getCount() : indexCount;
      queuedDrawCommand.instanceCount = 1;
      queuedDrawCommand.firstIndex = 0;
      queuedDrawCommand.baseVertex = allocationOffset / m_Stride;
      queuedDrawCommand.baseInstance = 0;
      m_QueuedDrawCommands.push_back(queuedDrawCommand);
      return true;
    }
    catch (const std::out_of_range& /* e */)
    {
      EN_CORE_ERROR("Could not find buffer region assocatied with ID {0}!", ID);
      return false;
    }
  }

  void MultiDrawArray::clear()
  {
    m_QueuedDrawCommands.clear();
  }

  int MultiDrawArray::size() const
  {
    int size = 0;
    for (const MemoryRegion& memoryRegion : m_MemoryPool)
      if (!memoryRegion.free)
        size++;
    return size;
  }

  float MultiDrawArray::usage() const
  {
    int usedMemory = 0;
    for (const MemoryRegion& memoryRegion : m_MemoryPool)
      if (!memoryRegion.free)
        usedMemory += memoryRegion.size;
    return static_cast<float>(usedMemory) / c_BufferSize;
  }

  float MultiDrawArray::fragmentation() const
  {
    uint32_t touchedMemory = m_MemoryPool.back().free ? m_MemoryPool.back().offset : c_BufferSize;

    uint32_t wastedMemory = 0;
    for (int i = 0; i < m_MemoryPool.size() - 1; ++i)
      if (m_MemoryPool[i].free)
        wastedMemory += m_MemoryPool[i].size;
    return static_cast<float>(wastedMemory) / touchedMemory;
  }

  const std::vector<DrawElementsIndirectCommand>& MultiDrawArray::getQueuedDrawCommands() const
  {
    return m_QueuedDrawCommands;
  }

  void MultiDrawArray::setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer)
  {
    m_VertexArray->setIndexBuffer(indexBuffer);
  }
}