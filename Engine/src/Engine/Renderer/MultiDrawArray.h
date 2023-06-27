#pragma once
#include "VertexArray.h"

namespace Engine
{
  template<typename T>
  class MultiArray
  {
  public:
    struct DrawCommand
    {
      uint32_t count;
      uint32_t instanceCount;
      uint32_t firstIndex;
      int baseVertex;
      uint32_t baseInstance;

      T ID;
      std::shared_ptr<std::size_t> commandIndex;

      DrawCommand(uint32_t indexCount, int vertexOffset, const T& identifier, std::size_t initialCommandIndex)
        : count(indexCount),
          instanceCount(1),
          firstIndex(0),
          baseVertex(vertexOffset),
          baseInstance(0),
          ID(identifier),
          commandIndex(std::make_shared<std::size_t>(initialCommandIndex)) {}
    };

  public:
    MultiArray(const BufferLayout& layout)
      : m_Capacity(64), m_Stride(layout.stride())
    {
      m_VertexArray = VertexArray::Create();
      m_VertexArray->setLayout(layout);
      m_VertexArray->setVertexBuffer(nullptr, m_Capacity * m_Stride);

      addFreeRegion(0, m_Capacity);
    }

    /*
        Sets an array of indices that represent the order in which vertices will be drawn.
        The count of this array should be a multiple of 3 (for drawing triangles).
        OpenGL expects vertices to be in counter-clockwise orientation.
    */
    void setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer)
    {
      m_VertexArray->setIndexBuffer(indexBuffer);
    }

    void bind() const { m_VertexArray->bind(); }
    void unBind() const { m_VertexArray->unBind(); }

    void add(const T& ID, const void* data, int vertexCount, uint32_t indexCount)
    {
      if (vertexCount == 0)
        return;

      EN_CORE_ASSERT(m_Allocations.find(ID) == m_Allocations.end(), "Memory has already been allocated for region with given ID!");

      FreeRegionsIterator bestFreeRegionPosition = m_FreeRegions.lower_bound(vertexCount);
      MemoryRegionsIterator bestMemoryRegion = m_MemoryRegions.end();

      if (bestFreeRegionPosition != m_FreeRegions.end())
      {
        int regionOffset = bestFreeRegionPosition->second;
        m_FreeRegions.erase(bestFreeRegionPosition);

        bestMemoryRegion = m_MemoryRegions.find(regionOffset);
        EN_CORE_ASSERT(bestMemoryRegion != m_MemoryRegions.end(), "No memory region was found at offset {0}!", regionOffset);
        EN_CORE_ASSERT(bestMemoryRegion->second.size >= vertexCount, "Memory region is not large enough to hold data!");
      }
      else
      {
        bestMemoryRegion = std::prev(m_MemoryRegions.end());
        if (!bestMemoryRegion->second.isFree())
        {
          // Add to memory regions but defer adding to free regions until we know final size
          auto [insertionPosition, insertionSuccess] = m_MemoryRegions.emplace(m_Capacity, 0);
          bestMemoryRegion = insertionPosition;
          EN_CORE_ASSERT(insertionSuccess, "Memory Region already exists at offset {0}!", m_Capacity);
        }
        else
          removeFromFreeRegionsMap(bestMemoryRegion);

        while (bestMemoryRegion->second.size < vertexCount)
        {
          uint32_t oldCapacity = m_Capacity;
          m_Capacity = static_cast<uint32_t>(c_CapacityIncreaseOnResize * m_Capacity);
          bestMemoryRegion->second.size += m_Capacity - oldCapacity;
        }

        m_VertexArray->resizeVertexBuffer(m_Capacity * m_Stride);
      }

      auto& [allocationOffset, allocationRegion] = *bestMemoryRegion;

      m_VertexArray->modifyVertexBuffer(data, allocationOffset * m_Stride, vertexCount * m_Stride);

      m_DrawCommands.emplace_back(indexCount, allocationOffset, ID, m_DrawCommands.size());
      m_Allocations.emplace(ID, m_DrawCommands.back().commandIndex);

      int memoryLeftover = allocationRegion.size - vertexCount;
      allocationRegion.ID = ID;
      allocationRegion.size = vertexCount;
      if (memoryLeftover)
        addFreeRegion(allocationOffset + allocationRegion.size, memoryLeftover);
    }

    void remove(const T& ID)
    {
      AllocationsIterator allocationToRemove = m_Allocations.find(ID);
      if (allocationToRemove == m_Allocations.end())
        return;

      std::size_t drawCommandIndex = *allocationToRemove->second;
      int freedRegionOffset = m_DrawCommands[drawCommandIndex].baseVertex;
      MemoryRegionsIterator freedRegionPosition = m_MemoryRegions.find(freedRegionOffset);
      EN_CORE_ASSERT(freedRegionPosition != m_MemoryRegions.end(), "No memory region was found at offset {0}!", freedRegionOffset);
      EN_CORE_ASSERT(!freedRegionPosition->second.isFree(), "Region is already free!");

      int freedRegionSize = freedRegionPosition->second.size;

      if (freedRegionPosition != m_MemoryRegions.begin())
      {
        MemoryRegionsIterator prevRegionPosition = std::prev(freedRegionPosition);
        const auto& [prevRegionOffset, prevRegion] = *prevRegionPosition;
        if (prevRegion.isFree())
        {
          freedRegionOffset = prevRegionOffset;
          freedRegionSize += prevRegion.size;

          removeFromFreeRegionsMap(prevRegionPosition);
          freedRegionPosition = m_MemoryRegions.erase(prevRegionPosition);
        }
      }

      MemoryRegionsIterator nextRegionPosition = m_MemoryRegions.erase(freedRegionPosition);
      if (nextRegionPosition != m_MemoryRegions.end())
      {
        const auto& [nextRegionOffset, nextRegion] = *nextRegionPosition;
        if (nextRegion.isFree())
        {
          freedRegionSize += nextRegion.size;

          removeFromFreeRegionsMap(nextRegionPosition);
          m_MemoryRegions.erase(nextRegionPosition);
        }
      }

      addFreeRegion(freedRegionOffset, freedRegionSize);

      std::swap(m_DrawCommands[drawCommandIndex], m_DrawCommands.back());
      *m_DrawCommands[drawCommandIndex].commandIndex = drawCommandIndex;
      m_DrawCommands.pop_back();

      m_Allocations.erase(allocationToRemove);
    }

    template<typename F, typename... Args>
    int mask(F function, Args&&... args)
    {
      int M = 0;
      int J = static_cast<int>(m_DrawCommands.size() - 1);

      while (M <= J)
      {
        while (function(m_DrawCommands[M].ID, args...) && M < J)
          M++;
        while (!function(m_DrawCommands[J].ID, args...) && M < J)
          J--;

        *m_DrawCommands[M].commandIndex = J;
        *m_DrawCommands[J].commandIndex = M;
        std::swap(m_DrawCommands[M++], m_DrawCommands[J--]);
      }
      return M;
    }

    const std::vector<DrawCommand>& getDrawCommandBuffer() const { return m_DrawCommands; }

    float usage() const
    {
      int dataSize = 0;
      for (const auto& [offset, region] : m_MemoryRegions)
        if (!region.isFree())
          dataSize += region.size;
      return static_cast<float>(dataSize) / m_Capacity;
    }

  private:
    struct MemoryRegion
    {
      std::optional<T> ID;
      int size;

      bool isFree() const { return !ID; }

      MemoryRegion(int regionSize)
        : ID(std::nullopt), size(regionSize) {}
    };

    using MemoryRegionsConstIterator = std::map<int, MemoryRegion>::const_iterator;
    using MemoryRegionsIterator = std::map<int, MemoryRegion>::iterator;
    using FreeRegionsIterator = std::multimap<int, int>::iterator;
    using AllocationsIterator = std::unordered_map<T, std::shared_ptr<std::size_t>>::iterator;

    static constexpr float c_CapacityIncreaseOnResize = 1.25f;

    std::unique_ptr<VertexArray> m_VertexArray;
    std::map<int, MemoryRegion> m_MemoryRegions;
    std::multimap<int, int> m_FreeRegions;
    std::unordered_map<T, std::shared_ptr<std::size_t>> m_Allocations;
    std::vector<DrawCommand> m_DrawCommands;
    uint32_t m_Capacity;
    uint32_t m_Stride;

    void addFreeRegion(int offset, int size)
    {
      m_MemoryRegions.emplace(offset, size);
      m_FreeRegions.emplace(size, offset);
    }

    void removeFromFreeRegionsMap(MemoryRegionsIterator it)
    {
      removeFromFreeRegionsMap(it->first, it->second.size);
    }

    void removeFromFreeRegionsMap(int offset, int size)
    {
      auto [begin, end] = m_FreeRegions.equal_range(size);
      for (FreeRegionsIterator it = begin; it != end; ++it)
        if (it->second == offset)
        {
          m_FreeRegions.erase(it);
          return;
        }
      EN_CORE_ERROR("No region at offset {0} of size {1} found!", offset, size);
    }
  };
}