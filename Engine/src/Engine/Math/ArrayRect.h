#pragma once
#include "ArrayBox.h"
#include "IBox2.h"

namespace eng::math
{
/*
  A 2D-style array that stores data on an integer lattice. Under the hood,
  the data is packed tightly in a single heap-allocated block of memory.
  Provides functions for operating on portions of data.

  Elements can be accessed with a 3D index. Alternatively, one can
  strip off portions of the array using square brackets.
*/
  template<typename T, std::integral IntType>
  class ArrayRect : private NonCopyable
  {
    IBox2<IntType> m_Bounds;
    i32 m_Stride;
    i32 m_Offset;
    std::unique_ptr<T[]> m_Data;

  public:
    using iterator = T*;
    using const_iterator = const T*;
    using Strip = ArrayBoxStrip<T, IntType>;

    ArrayRect(const IBox2<IntType>& bounds, AllocationPolicy policy)
    {
      setBounds(bounds);
      switch (policy)
      {
        case AllocationPolicy::Deferred:                                                  break;
        case AllocationPolicy::ForOverwrite:      allocate();                             break;
        case AllocationPolicy::DefaultInitialize: m_Data = std::make_unique<T[]>(size()); break;
      }
    }
    ArrayRect(const IBox2<IntType>& bounds, const T& initialValue)
      : ArrayRect(bounds, AllocationPolicy::ForOverwrite) { algo::fill(*this, initialValue); }

    operator bool() const { return static_cast<bool>(m_Data); }
    const T* data() const { return m_Data.get(); }

    T& operator()(const IVec2<IntType>& index) { ENG_MUTABLE_VERSION(operator(), index); }
    const T& operator()(const IVec2<IntType>& index) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(m_Bounds.encloses(index), "Index is out of bounds!");
      return m_Data[m_Stride * index.i + index.j - m_Offset];
    }

    Strip operator[](IntType index) { ENG_MUTABLE_VERSION(operator[], index); }
    const Strip operator[](IntType index) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(withinBounds(index, m_Bounds.min.i, m_Bounds.max.i + 1), "Index is out of bounds!");
      return Strip(m_Data.get() + m_Stride * (index - m_Bounds.min.i), m_Bounds.min.j);
    }

    iterator begin() { return m_Data.get();          }
    iterator end()   { return m_Data.get() + size(); }

    const_iterator begin() const { return m_Data.get();          }
    const_iterator end()   const { return m_Data.get() + size(); }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend()   const { return end();   }

    uSize size() const { return m_Bounds.volume(); }
    const IBox2<IntType>& bounds() const { return m_Bounds; }

    bool contains(const T& value) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::anyOf(*this, [&value](const T& data) { return data == value; });
    }

    bool filledWith(const T& value) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::allOf(*this, [&value](const T& data) { return data == value; });
    }

    bool contentsEqual(const IBox2<IntType>& compareSection, const ArrayBox<T, IntType>& container, const IBox2<IntType>& containerSection, const T& defaultValue) const
    {
      ENG_CORE_ASSERT(compareSection.extents() == containerSection.extents(), "Compared sections are not the same dimensions!");

      if (!m_Data && !container)
        return true;
      if (!m_Data)
        return container.allOf(containerSection, [&defaultValue](const T& value) { return value == defaultValue; });
      if (!container)
        return allOf(compareSection, [&defaultValue](const T& value) { return value == defaultValue; });

      IVec2<IntType> offset = containerSection.min - compareSection.min;
      return allOf(compareSection, [&container, &offset](const IVec2<IntType>& index, const T& data) { return data == container(index + offset); });
    }

    template<std::predicate<const T&> F>
    bool allOf(const IBox2<IntType>& section, F&& condition) const { return allOf(section, toIndexed(std::forward<F>(condition))); }

    template<std::predicate<const IVec2<IntType>&, const T&> F>
    bool allOf(F&& condition) const { return allOf(m_Bounds, std::forward<F>(condition)); }

    template<std::predicate<const IVec2<IntType>&, const T&> F>
    bool allOf(const IBox2<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::allOf(section, [this, &condition](const IVec2<IntType>& index) { return condition(index, (*this)(index)); });
    }

    template<std::predicate<const T&> F>
    bool anyOf(const IBox2<IntType>& section, F&& condition) const { return anyOf(section, toIndexed(std::forward<F>(condition))); }

    template<std::predicate<const IVec2<IntType>&, const T&> F>
    bool anyOf(F&& condition) const { return anyOf(m_Bounds, std::forward<F>(condition)); }

    template<std::predicate<const IVec2<IntType>&, const T&> F>
    bool anyOf(const IBox2<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::anyOf(section, [this, &condition](const IVec2<IntType>& index) { return condition(index, (*this)(index)); });
    }

    template<std::predicate<const T&> F>
    bool noneOf(const IBox2<IntType>& section, F&& condition) const { return noneOf(section, toIndexed(std::forward<F>(condition))); }

    template<std::predicate<const IVec2<IntType>&, const T&> F>
    bool noneOf(F&& condition) const { return noneOf(m_Bounds, std::forward<F>(condition)); }

    template<std::predicate<const IVec2<IntType>&, const T&> F>
    bool noneOf(const IBox2<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::noneOf(section, [this, &condition](const IVec2<IntType>& index) { return condition(index, (*this)(index)); });
    }

    void fill(const IBox2<IntType>& fillSection, const T& value)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      populate(fillSection, [&value](const IVec2<IntType>& index) { return value; });
    }

    void fill(const IBox2<IntType>& fillSection, const ArrayRect<T, IntType>& container, const IBox2<IntType>& containerSection)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(fillSection.extents() == containerSection.extents(), "Read and write sections are not the same dimensions!");

      IVec2<IntType> offset = containerSection.min - fillSection.min;
      populate(fillSection, [&container, &offset](const IVec2<IntType>& index) { return container(index + offset); });
    }

    template<InvocableWithReturnType<T, const IVec2<IntType>&> F>
    void populate(F&& function) { populate(m_Bounds, std::forward<F>(function)); }

    template<InvocableWithReturnType<T, const IVec2<IntType>&> F>
    void populate(const IBox2<IntType>& section, F&& function)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      for (const IVec2<IntType>& index : section)
        (*this)(index) = function(index);
    }

    template<std::invocable<const T&> F>
    void forEach(const IBox2<IntType>& section, F&& function) const { forEach(section, toIndexed(std::forward<F>(function))); }

    template<std::invocable<const IVec2<IntType>&, const T&> F>
    void forEach(F&& function) const { forEach(m_Bounds, std::forward<F>(function)); }

    template<std::invocable<const IVec2<IntType>&, const T&> F>
    void forEach(const IBox2<IntType>& section, F&& function) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      for (const IVec2<IntType>& index : section)
        function(index, (*this)(index));
    }

    void setBounds(const IBox2<IntType>& bounds)
    {
      m_Bounds = bounds;
      m_Stride = m_Bounds.extents().j;
      m_Offset = m_Stride * m_Bounds.min.i + m_Bounds.min.j;
    }

    void allocate()
    {
      if (m_Data)
        ENG_CORE_WARN("Data already allocated to ArrayRect. Ignoring...");
      else
        m_Data = std::make_unique_for_overwrite<T[]>(size());
    }

    void clear()
    {
      m_Data.reset();
    }

  private:
    template<std::invocable<T&> F>
    auto toIndexed(F&& function)
    {
      return [&function](const IVec2<IntType>& /*unused*/, T& data) { return function(data); };
    }

    template<std::invocable<const T&> F>
    auto toIndexed(F&& function) const
    {
      return [&function](const IVec2<IntType>& /*unused*/, const T& data) { return function(data); };
    }
  };
}