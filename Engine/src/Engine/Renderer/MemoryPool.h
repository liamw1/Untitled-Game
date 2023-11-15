#pragma once
#include "StorageBuffer.h"

namespace eng
{
  /*
    Helper class for managing GPU memory. Provides O(log(n) + m) insertion and removal, where
    n is the number of allocations and m is the mode of allocation sizes. Hence, this class is not
    suited for storing many identically-sized pieces of memory. Submitted data is stored in a single
    dynamically-resizing buffer on the GPU. The class tries to place memory into the buffer in a way
    that minimizes gaps between allocations, without dislocating existing allocations.

    NOTE: This class could be modified to remove the 'm' from the insertion/removal time complexity,
          but it's more trouble than it's worth at the moment as it hasn't been a problem yet.
    TODO: Maybe make this class safer?
  */
  class MemoryPool
  {
  public:
    using address_t = u32;

  private:
    struct MemoryRegion
    {
      bool free;
      i32 size;

      MemoryRegion(i32 regionSize)
        : free(true), size(regionSize) {}
    };

    std::shared_ptr<StorageBuffer> m_Buffer;
    std::map<address_t, MemoryRegion> m_Regions;  // For fast access based on address
    std::multimap<i32, address_t> m_FreeRegions;  // For fast access based on free region size
    i32 m_Capacity;

  public:
    MemoryPool();
    MemoryPool(StorageBuffer::Type bufferType, i32 initialCapacity = 64);

    void bind() const;
    void unBind() const;

    const std::shared_ptr<StorageBuffer>& buffer();

    /*
      Uploads data to GPU. May trigger a resize.
      \returns If a resize was triggered and an address for the allocated memory.
    */
    [[nodiscard]] std::pair<bool, address_t> add(const void* data, i32 size);

    /*
      Removes the memory at the specified address. Doesn't actually delete any memory.
      Instead, the memory is just no longer indexed by the memory pool and may be overwritten.
    */
    void remove(address_t address);

    /*
      Overwrites memory at the given address. Assumes data is the same size as the original
      allocation. Horrifically unsafe.
    */
    void amend(const void* data, address_t address);

  private:
    using RegionsIterator = std::map<address_t, MemoryRegion>::iterator;
    using FreeRegionsIterator = std::multimap<i32, address_t>::iterator;

    static constexpr f32 c_CapacityIncreaseOnResize = 1.25f;

    bool isFree(RegionsIterator region) const;
    i32& regionSize(RegionsIterator region);

    void addFreeRegion(address_t offset, i32 size);
    void removeFromFreeRegions(RegionsIterator it);
  };
}