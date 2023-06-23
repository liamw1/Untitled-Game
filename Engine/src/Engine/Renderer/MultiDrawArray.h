#pragma once
#include "VertexArray.h"

namespace Engine
{
  struct DrawElementsIndirectCommand
  {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int baseVertex;
    uint32_t baseInstance;
  };

  template<typename T>
  class MultiDrawArray
  {
  public:
    struct MemoryRegion
    {
      std::optional<T> ID;
      uint32_t offset;
      uint32_t size;
      uint32_t indexCount;

      MemoryRegion(uint32_t regionOffset, uint32_t regionSize)
        : ID(std::nullopt), offset(regionOffset), size(regionSize), indexCount(0) {}

      MemoryRegion(const T& allocationID, uint32_t regionOffset, uint32_t regionSize, uint32_t regionIndexCount)
        : ID(allocationID), offset(regionOffset), size(regionSize), indexCount(regionIndexCount) {}

      bool isFree() const { return !ID; }
    };

    using const_iterator = std::vector<MemoryRegion>::const_iterator;

    MultiDrawArray(const BufferLayout& layout)
    {
      m_VertexArray = VertexArray::Create();
      m_VertexArray->setLayout(layout);

      m_VertexArray->setVertexBuffer(nullptr, m_Capacity);

      m_MemoryPool.emplace_back(0, m_Capacity);
    }

    void bind() const
    {
      m_VertexArray->bind();
    }

    void unBind() const
    {
      m_VertexArray->unBind();
    }

    void add(const T& ID, const void* data, uint32_t size, uint32_t indexCount)
    {
      if (size == 0)
        return;

      EN_CORE_ASSERT(find(ID) == m_MemoryPool.end(), "Memory has already been allocated for region with ID {0}!", ID);

      std::optional<uint32_t> minimumMemoryLeftover = std::nullopt;
      iterator bestRegion = m_MemoryPool.begin();
      for (iterator memoryRegion = m_MemoryPool.begin(); memoryRegion != m_MemoryPool.end(); ++memoryRegion)
        if (memoryRegion->isFree() && memoryRegion->size > size)
        {
          uint32_t memoryLeftover = memoryRegion->size - size;
          if (!minimumMemoryLeftover || memoryLeftover < minimumMemoryLeftover)
          {
            minimumMemoryLeftover = memoryLeftover;
            bestRegion = memoryRegion;
          }
        }

      while (!minimumMemoryLeftover)
      {
        if (!m_MemoryPool.back().isFree())
          m_MemoryPool.emplace_back(m_Capacity, 0);
        m_MemoryPool.back().size += static_cast<uint32_t>((c_CapacityIncreaseOnResize - 1) * m_Capacity);

        m_Capacity = static_cast<uint32_t>(c_CapacityIncreaseOnResize * m_Capacity);
        m_VertexArray->resizeVertexBuffer(m_Capacity);

        bestRegion = std::prev(m_MemoryPool.end());
        if (bestRegion->size > size)
          minimumMemoryLeftover = bestRegion->size - size;
      }

      m_VertexArray->modifyVertexBuffer(data, bestRegion->offset, size);

      bestRegion->ID = ID;
      bestRegion->size = size;
      bestRegion->indexCount = indexCount;
      if (minimumMemoryLeftover > 0)
        m_MemoryPool.emplace(std::next(bestRegion), bestRegion->offset + size, *minimumMemoryLeftover);
    }

    void remove(const T& ID)
    {
      iterator freedRegion = find(ID);

      if (freedRegion == m_MemoryPool.end())
        return;

      freedRegion->ID = std::nullopt;

      if (freedRegion != m_MemoryPool.begin())
      {
        iterator prevRegion = std::prev(freedRegion);
        if (prevRegion->isFree())
        {
          freedRegion->offset = prevRegion->offset;
          freedRegion->size += prevRegion->size;
          freedRegion = m_MemoryPool.erase(prevRegion);
        }
      }

      iterator nextRegion = std::next(freedRegion);
      if (nextRegion != m_MemoryPool.end() && nextRegion->isFree())
      {
        freedRegion->size += nextRegion->size;
        m_MemoryPool.erase(nextRegion);
      }
    }

    uint32_t capacity()
    {
      return m_Capacity;
    }

    int size() const
    {
      int size = 0;
      for (const MemoryRegion& memoryRegion : m_MemoryPool)
        if (!memoryRegion.isFree())
          size++;
      return size;
    }

    float usage() const
    {
      int usedMemory = 0;
      for (const MemoryRegion& memoryRegion : m_MemoryPool)
        if (!memoryRegion.isFree())
          usedMemory += memoryRegion.size;
      return static_cast<float>(usedMemory) / m_Capacity;
    }

    float fragmentation() const
    {
      uint32_t touchedMemory = m_MemoryPool.back().isFree() ? m_MemoryPool.back().offset : m_Capacity;

      uint32_t wastedMemory = 0;
      for (int i = 0; i < m_MemoryPool.size() - 1; ++i)
        if (m_MemoryPool[i].isFree())
          wastedMemory += m_MemoryPool[i].size;
      return static_cast<float>(wastedMemory) / touchedMemory;
    }

    const_iterator begin() { return m_MemoryPool.begin(); }
    const_iterator end() { return m_MemoryPool.end(); }

    /*
      Sets an array of indices that represent the order in which vertices will be drawn.
      The count of this array should be a multiple of 3 (for drawing triangles).
      OpenGL expects vertices to be in counter-clockwise orientation.
    */
    void setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer)
    {
      m_VertexArray->setIndexBuffer(indexBuffer);
    }

  private:
    using iterator = std::vector<MemoryRegion>::iterator;

    static constexpr float c_CapacityIncreaseOnResize = 1.25f;

    std::unique_ptr<VertexArray> m_VertexArray;
    std::vector<MemoryRegion> m_MemoryPool;
    uint32_t m_Capacity = 64;

    iterator find(const T& ID)
    {
      return std::find_if(m_MemoryPool.begin(), m_MemoryPool.end(), [&ID](const MemoryRegion& memoryRegion)->bool { return memoryRegion.ID == ID; });
    }
  };
}