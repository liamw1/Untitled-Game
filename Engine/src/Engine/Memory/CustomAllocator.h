#pragma once

namespace eng::mem
{
  template<typename T>
  concept DeallocatorPayload = std::invocable<T> && std::equality_comparable<T> && std::copyable<T>;

  /*
    A custom allocator that executes extra code when the object is deallocated.
    The first template argument is the payload, which must be a callable that
    is equality comparable and copyable. The second template argument is the
    object type that is allocated.
  */
  template<DeallocatorPayload T, typename V>
  class UponDeallocation
  {
    T m_Payload;

  public:
    // Necessary to prevent compiler issues due to multiple template parameters. Not sure if guaranteed to work by standard
    template<typename U>
    struct rebind { using other = UponDeallocation<T, U>; };

    // Necessary to meet requirements of an Allocator
    using value_type = V;

    UponDeallocation() = default;
  
    template<typename... Args>
      requires std::is_constructible_v<T, Args...>
    UponDeallocation(Args&&... args)
      : m_Payload(std::forward<Args>(args)...) {}

    template<typename U>
    UponDeallocation(const UponDeallocation<T, U>& other)
      : m_Payload(other.payload()) {}

    const T& payload() const { return m_Payload; }

    value_type* allocate(uSize n)
    {
      if (n > std::numeric_limits<uSize>::max() / sizeof(value_type))
        throw std::bad_array_new_length();

      if (value_type* allocationAddress = static_cast<value_type*>(std::malloc(n * sizeof(value_type))))
        return allocationAddress;

      throw std::bad_alloc();
    }

    void deallocate(value_type* allocationAddress, uSize n)
    {
      m_Payload();
      std::free(allocationAddress);
    }

    template<typename U>
    bool operator==(const UponDeallocation<T, U>& other)
    {
      return m_Payload == other.payload();
    }

    template<typename U>
    bool operator!=(const UponDeallocation<T, U>& other) { return !(*this == other); }
  };
}