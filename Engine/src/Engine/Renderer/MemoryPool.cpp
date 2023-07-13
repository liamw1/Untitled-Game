#include "ENpch.h"
#include "MemoryPool.h"

namespace Engine
{
  MemoryPool::MemoryPool(StorageBuffer::Type bufferType, int initialCapacity)
    : m_Capacity(initialCapacity)
  {
    m_Buffer = StorageBuffer::Create(bufferType);
    m_Buffer->set(nullptr, m_Capacity);

    addFreeRegion(0, m_Capacity);
  }

  void MemoryPool::bind() const
  {
    m_Buffer->bind();
  }

  void MemoryPool::unBind() const
  {
    m_Buffer->unBind();
  }

  const std::shared_ptr<StorageBuffer>& MemoryPool::buffer()
  {
    return m_Buffer;
  }

  std::pair<bool, size_t> MemoryPool::add(const void* data, int size)
  {
    if (size <= 0)
      return { false, -1 };

    bool triggeredResize = false;
    FreeRegionsIterator bestFreeRegionPosition = m_FreeRegions.lower_bound(size);
    RegionsIterator bestRegionPosition = m_Regions.end();
    if (bestFreeRegionPosition != m_FreeRegions.end())
    {
      size_t regionAddress = bestFreeRegionPosition->second;
      m_FreeRegions.erase(bestFreeRegionPosition);

      bestRegionPosition = m_Regions.find(regionAddress);
      EN_CORE_ASSERT(bestRegionPosition != m_Regions.end(), "No memory region was found at offset {0}!", regionAddress);
      EN_CORE_ASSERT(regionSize(bestRegionPosition) >= size, "Memory region is not large enough to hold data!");
    }
    else
    {
      bestRegionPosition = std::prev(m_Regions.end());
      if (!isFree(bestRegionPosition))
      {
        // Add to memory regions but defer adding to free regions until we know final size
        auto [insertionPosition, insertionSuccess] = m_Regions.emplace(m_Capacity, 0);
        bestRegionPosition = insertionPosition;
        EN_CORE_ASSERT(insertionSuccess, "Memory Region already exists at offset {0}!", m_Capacity);
      }
      else
        removeFromFreeRegions(bestRegionPosition);

      while (regionSize(bestRegionPosition) < size)
      {
        int oldCapacity = m_Capacity;
        m_Capacity = static_cast<int>(c_CapacityIncreaseOnResize * m_Capacity);
        regionSize(bestRegionPosition) += m_Capacity - oldCapacity;
      }

      m_Buffer->resize(m_Capacity);
      triggeredResize = true;
    }

    auto& [allocationAddress, allocationRegion] = *bestRegionPosition;

    int memoryLeftover = allocationRegion.size - size;
    if (memoryLeftover > 0)
      addFreeRegion(allocationAddress + size, memoryLeftover);
    allocationRegion.free = false;
    allocationRegion.size = size;

    m_Buffer->update(data, allocationAddress, size);
    return { triggeredResize, allocationAddress };
  }

  void MemoryPool::remove(size_t address)
  {
    RegionsIterator allocationPosition = m_Regions.find(address);
    EN_CORE_ASSERT(allocationPosition != m_Regions.end(), "No memory region was found at adress {0}!", address);
    EN_CORE_ASSERT(!isFree(allocationPosition), "Region is already free!");

    int freedRegionSize = regionSize(allocationPosition);
    size_t freedRegionAddress = address;
    RegionsIterator freedRegionPosition = allocationPosition;

    // If previous region is free, merge with newly freed region
    if (allocationPosition != m_Regions.begin())
    {
      RegionsIterator prevRegionPosition = std::prev(allocationPosition);
      auto [prevRegionAddress, prevRegion] = *prevRegionPosition;
      if (prevRegion.free)
      {
        freedRegionAddress = prevRegionAddress;
        freedRegionSize += prevRegion.size;

        removeFromFreeRegions(prevRegionPosition);
        freedRegionPosition = m_Regions.erase(prevRegionPosition);
      }
    }

    // If next region if free, merge with newly freed region
    RegionsIterator nextRegionPosition = m_Regions.erase(freedRegionPosition);
    if (nextRegionPosition != m_Regions.end())
    {
      auto [nextRegionAddress, nextRegion] = *nextRegionPosition;
      if (nextRegion.free)
      {
        freedRegionSize += nextRegion.size;

        removeFromFreeRegions(nextRegionPosition);
        m_Regions.erase(nextRegionPosition);
      }
    }

    // Update free regions container
    addFreeRegion(freedRegionAddress, freedRegionSize);
  }

  void MemoryPool::amend(const void* data, size_t address)
  {
    RegionsIterator allocationPosition = m_Regions.find(address);
    m_Buffer->update(data, address, regionSize(allocationPosition));
  }

  bool MemoryPool::isFree(RegionsIterator regionIterator) const
  {
    return regionIterator->second.free;
  }

  int& MemoryPool::regionSize(RegionsIterator regionIterator)
  {
    return regionIterator->second.size;
  }

  void MemoryPool::addFreeRegion(size_t address, int size)
  {
    m_Regions.emplace(address, size);
    m_FreeRegions.emplace(size, address);
  }

  void MemoryPool::removeFromFreeRegions(RegionsIterator regionIterator)
  {
    auto [address, region] = *regionIterator;

    auto [begin, end] = m_FreeRegions.equal_range(region.size);
    for (FreeRegionsIterator it = begin; it != end; ++it)
      if (it->second == address)
      {
        m_FreeRegions.erase(it);
        return;
      }
    EN_CORE_ERROR("No region at address {0} of size {1} found!", address, region.size);
  }
}