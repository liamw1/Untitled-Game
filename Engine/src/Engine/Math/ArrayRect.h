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
    T* m_Data;

  public:
    using Strip = ArrayBoxStrip<T, IntType>;

    ArrayRect(const IBox2<IntType>& bounds, AllocationPolicy policy)
      : m_Data(nullptr)
    {
      setBounds(bounds);
      if (policy != AllocationPolicy::Deferred)
        allocate();
      if (policy == AllocationPolicy::DefaultInitialize)
        fill(T());
    }
    ArrayRect(const IBox2<IntType>& bounds, const T& initialValue)
      : ArrayRect(bounds, AllocationPolicy::ForOverwrite) { fill(initialValue); }
    ~ArrayRect() { delete[] m_Data; }

    ArrayRect(ArrayRect&& other) noexcept = default;
    ArrayRect& operator=(ArrayRect&& other) noexcept
    {
      if (&other != this)
      {
        m_Bounds = other.m_Bounds;
        m_Stride = other.m_Stride;
        m_Offset = other.m_Offset;

        delete[] m_Data;
        m_Data = other.m_Data;
        other.m_Data = nullptr;
      }
      return *this;
    }

    operator bool() const { return m_Data; }
    const T* data() const { return m_Data; }

    T& operator()(const IVec2<IntType>& index) { return ENG_MUTABLE_VERSION(operator(), index); }
    const T& operator()(const IVec2<IntType>& index) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(m_Bounds.encloses(index), "Index is out of bounds!");
      return m_Data[m_Stride * index.i + index.j - m_Offset];
    }

    Strip operator[](IntType index) { return static_cast<const ArrayRect&>(*this).operator[](index); }
    const Strip operator[](IntType index) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(debug::BoundsCheck(index, m_Bounds.min.i, m_Bounds.max.i + 1), "Index is out of bounds!");
      return Strip(m_Data + m_Stride * (index - m_Bounds.min.i), m_Bounds.min.j);
    }

    size_t size() const { return m_Bounds.volume(); }
    const IBox2<IntType>& bounds() const { return m_Bounds; }

    bool contains(const T& value) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return std::any_of(m_Data, m_Data + size(), [&value](const T& data)
      {
        return data == value;
      });
    }

    bool filledWith(const T& value) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return std::all_of(m_Data, m_Data + size(), [&value](const T& data)
      {
        return data == value;
      });
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
      return compareSection.allOf([this, &container, &offset](const IVec2<IntType>& index)
      {
        return (*this)(index) == container(index + offset);
      });
    }

    template<InvocableWithReturnType<bool, T> F>
    bool allOf(const IBox2<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return section.allOf([this, &condition](const IVec2<IntType>& index)
      {
        return condition((*this)(index));
      });
    }

    template<InvocableWithReturnType<bool, T> F>
    bool anyOf(const IBox2<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return section.anyOf([this, &condition](const IVec2<IntType>& index)
      {
        return condition((*this)(index));
      });
    }

    template<InvocableWithReturnType<bool, T> F>
    bool noneOf(const IBox2<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return section.noneOf([this, &condition](const IVec2<IntType>& index)
      {
        return condition((*this)(index));
      });
    }

    void fill(const T& value)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      std::fill_n(m_Data, size(), value);
    }

    void fill(const IBox2<IntType>& fillSection, const T& value)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      fillSection.forEach([this, &value](const IVec2<IntType>& index)
      {
        (*this)(index) = value;
      });
    }

    void fill(const IBox2<IntType>& fillSection, const ArrayBox<T, IntType>& container, const IBox2<IntType>& containerSection)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(fillSection.extents() == containerSection.extents(), "Read and write sections are not the same dimensions!");

      IVec2<IntType> offset = containerSection.min - fillSection.min;
      fillSection.forEach([this, &container, &offset](const IVec2<IntType>& index)
      {
        (*this)(index) = container(index + offset);
      });
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
        m_Data = new T[size()];
    }

    void reset()
    {
      delete[] m_Data;
      m_Data = nullptr;
    }
  };
}