#include "ENpch.h"
#include "MemoryPool.h"
#include "Engine/Core/Casting.h"
#include "Engine/Debug/Assert.h"

namespace eng
{
  MemoryPool::MemoryPool()
    : m_Capacity(0) {}
  MemoryPool::MemoryPool(StorageBuffer::Type bufferType, i32 initialCapacity)
    : m_Capacity(initialCapacity)
  {
    m_Buffer = StorageBuffer::Create(bufferType);
    m_Buffer->resize(m_Capacity);

    addFreeRegion(0, m_Capacity);
  }

  void MemoryPool::bind() const { m_Buffer->bind(); }
  void MemoryPool::unBind() const { m_Buffer->unBind(); }
  const std::shared_ptr<StorageBuffer>& MemoryPool::buffer() { return m_Buffer; }

  std::pair<bool, MemoryPool::address_t> MemoryPool::malloc(const mem::Data& data)
  {
    if (data.size() == 0)
      return { false, -1 };

    i32 size = arithmeticCast<i32>(data.size());

    // Find the smallest free region that can fit the data. If no such region can be found, resize.
    bool triggeredResize = false;
    FreeRegionsIterator bestFreeRegionPosition = m_FreeRegions.lower_bound(size);
    RegionsIterator bestRegionPosition = m_Regions.end();
    if (bestFreeRegionPosition != m_FreeRegions.end())
    {
      address_t regionAddress = bestFreeRegionPosition->second;
      m_FreeRegions.erase(bestFreeRegionPosition);

      bestRegionPosition = m_Regions.find(regionAddress);
      ENG_CORE_ASSERT(bestRegionPosition != m_Regions.end(), "No memory region was found at offset {0}!", regionAddress);
      ENG_CORE_ASSERT(regionSize(bestRegionPosition) >= size, "Memory region is not large enough to hold data!");
    }
    else
    {
      bestRegionPosition = std::prev(m_Regions.end());
      if (!isFree(bestRegionPosition))
      {
        // Add to memory regions but defer adding to free regions until we know final size
        auto [insertionPosition, insertionSuccess] = m_Regions.emplace(m_Capacity, 0);
        bestRegionPosition = insertionPosition;
        ENG_CORE_ASSERT(insertionSuccess, "Memory Region already exists at offset {0}!", m_Capacity);
      }
      else
        removeFromFreeRegions(bestRegionPosition);

      while (regionSize(bestRegionPosition) < size)
      {
        i32 oldCapacity = m_Capacity;
        m_Capacity = arithmeticCast<i32>(c_CapacityIncreaseOnResize * m_Capacity);
        regionSize(bestRegionPosition) += m_Capacity - oldCapacity;
      }

      m_Buffer->resize(m_Capacity);
      triggeredResize = true;
    }

    auto& [allocationAddress, allocationRegion] = *bestRegionPosition;

    // Add any leftover memory to free regions.
    i32 memoryLeftover = allocationRegion.size - size;
    if (memoryLeftover > 0)
      addFreeRegion(allocationAddress + size, memoryLeftover);
    allocationRegion.free = false;
    allocationRegion.size = size;

    // Upload data to GPU
    m_Buffer->modify(allocationAddress, data);
    return { triggeredResize, allocationAddress };
  }

  void MemoryPool::free(address_t address)
  {
    RegionsIterator freedRegionPosition = m_Regions.find(address);
    ENG_CORE_ASSERT(freedRegionPosition != m_Regions.end(), "No memory region was found at adress {0}!", address);
    ENG_CORE_ASSERT(!isFree(freedRegionPosition), "Region is already free!");

    // If previous region is free, merge with newly freed region
    if (freedRegionPosition != m_Regions.begin() && isFree(std::prev(freedRegionPosition)))
      freedRegionPosition = mergeToPreviousRegion(freedRegionPosition);

    // If next region if free, merge with newly freed region
    RegionsIterator nextRegionPosition = std::next(freedRegionPosition);
    if (nextRegionPosition != m_Regions.end() && isFree(nextRegionPosition))
      freedRegionPosition = mergeToPreviousRegion(nextRegionPosition);

    // Mark region as free and update free regions container
    isFree(freedRegionPosition) = true;
    m_FreeRegions.emplace(regionSize(freedRegionPosition), freedRegionPosition->first);
  }

  std::pair<bool, MemoryPool::address_t> MemoryPool::realloc(address_t address, const mem::Data& data)
  {
    RegionsIterator allocationPosition = m_Regions.find(address);
    ENG_CORE_ASSERT(allocationPosition != m_Regions.end(), "No memory region was found at adress {0}!", address);
    ENG_CORE_ASSERT(!isFree(allocationPosition), "Region at address {0} has already been freed!", address);

    i32 size = arithmeticCast<i32>(data.size());
    if (size != regionSize(allocationPosition))
    {
      free(address);
      return malloc(data);
    }

    m_Buffer->modify(address, data);
    return { false, address };
  }

  bool& MemoryPool::isFree(RegionsIterator regionIterator) const
  {
    return regionIterator->second.free;
  }

  i32& MemoryPool::regionSize(RegionsIterator regionIterator)
  {
    return regionIterator->second.size;
  }

  void MemoryPool::addFreeRegion(address_t address, i32 size)
  {
    m_Regions.emplace(address, size);
    m_FreeRegions.emplace(size, address);
  }

  void MemoryPool::removeFromFreeRegions(RegionsIterator regionIterator)
  {
    const auto& [address, region] = *regionIterator;

    auto [begin, end] = m_FreeRegions.equal_range(region.size);
    FreeRegionsIterator removalPosition = std::find_if(begin, end, [address](std::pair<i32, address_t> freeRegion) { return freeRegion.second == address; });
    if (removalPosition == end)
      ENG_CORE_ERROR("No region at address {0} of size {1} found!", address, region.size);
    else
      m_FreeRegions.erase(removalPosition);
  }

  MemoryPool::RegionsIterator MemoryPool::mergeToPreviousRegion(RegionsIterator regionIterator)
  {
    if (regionIterator == m_Regions.begin() || regionIterator == m_Regions.end())
      return regionIterator;

    RegionsIterator prevRegionPosition = std::prev(regionIterator);
    if (isFree(regionIterator))
      removeFromFreeRegions(regionIterator);
    if (isFree(prevRegionPosition))
      removeFromFreeRegions(prevRegionPosition);
    isFree(prevRegionPosition) = false;

    regionSize(prevRegionPosition) += regionSize(regionIterator);
    return std::prev(m_Regions.erase(regionIterator));
  }
}