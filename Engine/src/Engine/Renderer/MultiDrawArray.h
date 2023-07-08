#pragma once
#include "VertexArray.h"

namespace Engine
{
  template<typename Identifier, typename Derived>
  class MultiDrawCommand
  {
  public:
    MultiDrawCommand(const Identifier& id, uint32_t vertexCount)
      : m_VertexCount(vertexCount),
        m_InstanceCount(1),
        m_FirstVertex(0),
        m_BaseInstance(0),
        m_ID(id),
        m_CommandIndex(nullptr) {}

    uint32_t vertexCount() const { return m_VertexCount; }

    uint32_t firstVertex() const { return m_FirstVertex; }

    const Identifier& id() const { return m_ID; }

    const std::shared_ptr<std::size_t>& commandIndex() const { return m_CommandIndex; }

    void setPlacement(uint32_t firstVertex, const std::shared_ptr<std::size_t>& commandIndex)
    {
      m_FirstVertex = firstVertex;
      m_CommandIndex = commandIndex;
    }

    const void* vertexData() const { return static_cast<Derived*>(this)->vertexData(); }

  protected:
    uint32_t m_VertexCount;
    uint32_t m_InstanceCount;
    uint32_t m_FirstVertex;
    uint32_t m_BaseInstance;

    Identifier m_ID;
    std::shared_ptr<std::size_t> m_CommandIndex;
  };

  template<typename DrawCommand>
  class MultiDrawArray
  {
  public:
    using Identifier = std::decay_t<decltype(std::declval<DrawCommand>().id())>;

    MultiDrawArray(const BufferLayout& layout)
      : m_Capacity(c_InitialCapacity), m_Stride(layout.stride())
    {
      m_VertexArray = VertexArray::Create();
      m_VertexArray->setVertexBuffer(nullptr, m_Capacity * m_Stride);
      m_VertexArray->setLayout(layout);

      addFreeRegion(0, m_Capacity);
    }

    void bind() const { m_VertexArray->bind(); }
    void unBind() const { m_VertexArray->unBind(); }

    void add(DrawCommand&& drawCommand)
    {
      int vertexCount = drawCommand.vertexCount();
      if (vertexCount == 0)
        return;

      EN_CORE_ASSERT(m_Allocations.find(drawCommand.id()) == m_Allocations.end(), "Memory has already been allocated for region with given ID!");

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

      m_VertexArray->updateVertexBuffer(drawCommand.vertexData(), allocationOffset * m_Stride, vertexCount * m_Stride);

      int memoryLeftover = allocationRegion.size - vertexCount;
      allocationRegion.id = drawCommand.id();
      allocationRegion.size = vertexCount;
      if (memoryLeftover)
        addFreeRegion(allocationOffset + allocationRegion.size, memoryLeftover);

      std::shared_ptr<std::size_t> commandIndex = std::make_shared<std::size_t>(m_DrawCommands.size());
      m_Allocations.emplace(drawCommand.id(), commandIndex);
      drawCommand.setPlacement(allocationOffset, commandIndex);
      m_DrawCommands.push_back(std::move(drawCommand));
    }

    void remove(const Identifier& id)
    {
      AllocationsIterator allocationToRemove = m_Allocations.find(id);
      if (allocationToRemove == m_Allocations.end())
        return;

      std::size_t drawCommandIndex = *allocationToRemove->second;
      int freedRegionOffset = m_DrawCommands[drawCommandIndex].firstVertex();
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
      *m_DrawCommands[drawCommandIndex].commandIndex() = drawCommandIndex;
      m_DrawCommands.pop_back();

      m_Allocations.erase(allocationToRemove);
    }

    template<typename F, typename... Args>
    int mask(F condition, Args&&... args)
    {
      int leftIndex = 0;
      int rightIndex = static_cast<int>(m_DrawCommands.size() - 1);

      while (leftIndex < rightIndex)
      {
        while (condition(m_DrawCommands[leftIndex].id(), std::forward<Args>(args)...) && leftIndex < rightIndex)
          leftIndex++;
        while (!condition(m_DrawCommands[rightIndex].id(), std::forward<Args>(args)...) && leftIndex < rightIndex)
          rightIndex--;

        if (leftIndex != rightIndex)
        {
          std::swap(m_DrawCommands[leftIndex], m_DrawCommands[rightIndex]);
          std::swap(*m_DrawCommands[leftIndex].commandIndex(), *m_DrawCommands[rightIndex].commandIndex());
        }
      }
      return leftIndex;
    }

    template<typename F, typename... Args>
    void sort(int drawCount, F comparision, Args&&... args)
    {
      std::sort(m_DrawCommands.begin(), m_DrawCommands.begin() + drawCount, [&](const DrawCommand& drawA, const DrawCommand& drawB)
        {
          return comparision(drawA.id(), drawB.id(), std::forward<Args>(args)...);
        });

      for (int i = 0; i < drawCount; ++i)
        *m_DrawCommands[i].commandIndex() = i;
    }

    template<typename F, typename... Args>
    void amend(int drawCount, F function, Args&&... args)
    {
      for (auto it = m_DrawCommands.begin(); it != m_DrawCommands.begin() + drawCount; ++it)
      {
        int oldVertexCount = it->vertexCount();
        if (function(*it, std::forward<Args>(args)...))
        {
          if (it->vertexCount() > oldVertexCount)
          {
            EN_CORE_ERROR("Ammended draw command added additional vertices! Discarding changes...");
            continue;
          }
          uploadVertexData(*it);
        }
      }
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
      std::optional<Identifier> id;
      int size;

      bool isFree() const { return !id; }

      MemoryRegion(int regionSize)
        : id(std::nullopt), size(regionSize) {}
    };

    using MemoryRegionsConstIterator = std::map<int, MemoryRegion>::const_iterator;
    using MemoryRegionsIterator = std::map<int, MemoryRegion>::iterator;
    using FreeRegionsIterator = std::multimap<int, int>::iterator;
    using AllocationsIterator = std::unordered_map<Identifier, std::shared_ptr<std::size_t>>::iterator;

    static constexpr int c_InitialCapacity = 64;
    static constexpr float c_CapacityIncreaseOnResize = 1.25f;

    std::unique_ptr<VertexArray> m_VertexArray;
    std::map<int, MemoryRegion> m_MemoryRegions;
    std::multimap<int, int> m_FreeRegions;
    std::unordered_map<Identifier, std::shared_ptr<std::size_t>> m_Allocations;
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

    void uploadVertexData(const DrawCommand& drawCommand)
    {
      m_VertexArray->updateVertexBuffer(drawCommand.vertexData(), drawCommand.firstVertex() * m_Stride, drawCommand.vertexCount() * m_Stride);
    }
  };
}