#pragma once
#include "Engine/Core/FixedWidthTypes.h"
#include "Engine/Core/Policy.h"
#include "Engine/Debug/Assert.h"

namespace eng
{
  template<typename T, uSize N>
  class UniqueArray
  {
    std::unique_ptr<T[]> m_Data;

  public:
    using iterator = T*;
    using const_iterator = const T*;

    UniqueArray(AllocationPolicy policy)
    {
      switch (policy)
      {
        case AllocationPolicy::Deferred:                                              break;
        case AllocationPolicy::ForOverwrite:      allocate();                         break;
        case AllocationPolicy::DefaultInitialize: m_Data = std::make_unique<T[]>(N);  break;
      }
    }

    operator bool() const { return static_cast<bool>(m_Data); }

    T& operator[](uSize index) { ENG_MUTABLE_VERSION(operator[], index); }
    const T& operator[](uSize index) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(withinBounds(index, 0, N), "Index is out of bounds!");
      return m_Data[index];
    }

    iterator begin() { return m_Data.get();     }
    iterator end()   { return m_Data.get() + N; }

    const_iterator begin() const { return m_Data.get();     }
    const_iterator end()   const { return m_Data.get() + N; }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend()   const { return end();   }

    constexpr uSize size() const { return m_Data.size(); }

    void allocate() { m_Data = std::make_unique_for_overwrite<T[]>(N); }
    void reset()
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      m_Data.reset();
    }
  };
}