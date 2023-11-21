#pragma once
#include "StorageBuffer.h"

namespace eng::mem
{
  /*
    Helper class for managing GPU memory. Provides O(log(n) + m) insertion and removal, where
    n is the number of allocations and m is the mode of allocation sizes. Hence, this class is not
    suited for storing many identically-sized pieces of memory. Submitted data is stored in a single
    dynamically-resizing buffer on the GPU. The class tries to place memory into the buffer in a way
    that minimizes gaps between allocations, without dislocating existing allocations.

    NOTE: This class could be modified to remove the 'm' from the insertion/removal time complexity,
          but it's more trouble than it's worth at the moment as it hasn't been a problem yet.
  */
  class MemoryPool
  {
  public:
    using address_t = u32;
    struct AllocationResult
    {
      address_t address;
      bool bufferResized;
    };

  private:
    struct MemoryRegion
    {
      i32 size;
      bool free;

      MemoryRegion(i32 regionSize)
        : free(true), size(regionSize) {}
    };

    static constexpr f32 c_CapacityIncreaseOnResize = 1.25f;

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

    bool validAllocation(address_t address) const;

    /*
      Uploads data to GPU. May trigger a resize.
      \returns If a resize was triggered and an address for the allocated memory.
    */
    [[nodiscard]] AllocationResult malloc(const mem::Data& data);

    /*
      Removes the memory at the specified address. Doesn't actually delete any memory.
      Instead, the memory is just no longer indexed by the memory pool and may be overwritten.
    */
    void free(address_t address);

    /*
      Overwrites data that was previously uploaded to the GPU. May trigger a resize.
      \returns If a resize was triggered and an address for the reallocated memory.
    */
    [[nodiscard]] AllocationResult realloc(address_t address, const mem::Data& data);

  private:
    using RegionsIterator = std::map<address_t, MemoryRegion>::iterator;
    using FreeRegionsIterator = std::multimap<i32, address_t>::iterator;

    bool& isFree(RegionsIterator regionIterator) const;
    i32& regionSize(RegionsIterator regionIterator);

    void addFreeRegion(address_t offset, i32 size);
    void removeFromFreeRegions(RegionsIterator regionIterator);

    /*
      Attempts to merge given region with the previous one. If either region is free, removes them
      from the free region map and marks the resulting region as not free. If the merge is not
      possible because a begin or end iterator was given, does nothing.

      \returns The newly merged region if the merge was successful and the original region otherwise.
    */
    RegionsIterator mergeToPreviousRegion(RegionsIterator regionIterator);
  };
}