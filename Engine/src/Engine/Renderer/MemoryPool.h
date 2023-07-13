#pragma once
#include "StorageBuffer.h"
#include <map>

namespace Engine
{
  class MemoryPool
  {
  public:
    MemoryPool(StorageBuffer::Type bufferType, int initialCapacity = 64);

    void bind() const;
    void unBind() const;

    const std::shared_ptr<StorageBuffer>& buffer();

    [[nodiscard]] std::pair<bool, size_t> add(const void* data, int size);

    void remove(size_t address);

    void amend(const void* data, size_t address);

  private:
    struct MemoryRegion
    {
      bool free;
      int size;

      MemoryRegion(int regionSize)
        : free(true), size(regionSize) {}
    };

    using RegionsIterator = std::map<size_t, MemoryRegion>::iterator;
    using FreeRegionsIterator = std::multimap<int, size_t>::iterator;

    static constexpr float c_CapacityIncreaseOnResize = 1.25f;

    std::shared_ptr<StorageBuffer> m_Buffer;
    std::map<size_t, MemoryRegion> m_Regions;         // For fast access based on address
    std::multimap<int, size_t> m_FreeRegions;         // For fast access based on free region size
    int m_Capacity;

    bool isFree(RegionsIterator region) const;
    int& regionSize(RegionsIterator region);

    void addFreeRegion(size_t offset, int size);
    void removeFromFreeRegions(RegionsIterator it);
  };
}