#pragma once
#include "IBox2.h"
#include "IBox3.h"
#include "Engine/Core/Algorithm.h"
#include "Engine/Core/Policy.h"
#include "Engine/Utilities/Constraints.h"

namespace eng::math
{
  template<typename T, std::integral IntType>
  class ArrayBoxStrip
  {
    T* m_Begin;
    i32 m_Offset;

  public:
    ArrayBoxStrip(T* begin, i32 offset)
      : m_Begin(begin), m_Offset(offset) {}

    T& operator[](IntType index) { ENG_MUTABLE_VERSION(operator[], index); }
    const T& operator[](IntType index) const { return m_Begin[index - m_Offset]; }

  private:
    ~ArrayBoxStrip() = default;
  };

  template<typename T, std::integral IntType>
  class ArrayBoxLayer
  {
    T* m_Begin;
    IBox2<IntType> m_Bounds;

  public:
    ArrayBoxLayer(T* begin, const IBox2<IntType>& bounds)
      : m_Begin(begin), m_Bounds(bounds) {}

    ArrayBoxStrip<T, IntType> operator[](IntType index) { ENG_MUTABLE_VERSION(operator[], index); }
    const ArrayBoxStrip<T, IntType> operator[](IntType index) const
    {
      ENG_CORE_ASSERT(debug::boundsCheck(index, m_Bounds.min.i, m_Bounds.max.i + 1), "Index is out of bounds!");
      return ArrayBoxStrip<T, IntType>(m_Begin + m_Bounds.extents().j * (index - m_Bounds.min.i), m_Bounds.min.j);
    }

  private:
    ~ArrayBoxLayer() = default;
  };



  /*
    A 3D-style array that stores data on an integer lattice. Under the hood,
    the data is packed tightly in a single heap-allocated block of memory.
    Provides functions for operating on portions of data.

    Elements can be accessed with a 3D index. Alternatively, one can strip off
    portions of the array using square brackets. For instance, arr[i] gives a
    2D layer and arr[i][j] gives a 1D strip.
  */
  template<typename T, std::integral IntType>
  class ArrayBox : private NonCopyable
  {
    IBox3<IntType> m_Bounds;
    IVec2<iSize> m_Strides;
    iSize m_Offset;
    std::unique_ptr<T[]> m_Data;

  public:
    using Layer = ArrayBoxLayer<T, IntType>;
    using Strip = ArrayBoxStrip<T, IntType>;

    ArrayBox(const IBox3<IntType>& bounds, AllocationPolicy policy)
    {
      setBounds(bounds);
      switch (policy)
      {
        case AllocationPolicy::Deferred:                                                  break;
        case AllocationPolicy::ForOverwrite:      allocate();                             break;
        case AllocationPolicy::DefaultInitialize: m_Data = std::make_unique<T[]>(size()); break;
      }
    }
    ArrayBox(const IBox3<IntType>& bounds, const T& initialValue)
      : ArrayBox(bounds, AllocationPolicy::ForOverwrite) { fill(initialValue); }

    operator bool() const { return static_cast<bool>(m_Data); }
    const T* data() const { return m_Data.get(); }

    T& operator()(const IVec3<IntType>& index) { ENG_MUTABLE_VERSION(operator(), index); }
    const T& operator()(const IVec3<IntType>& index) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(m_Bounds.encloses(index), "Index is out of bounds!");
      return m_Data[m_Strides.i * index.i + m_Strides.j * index.j + index.k - m_Offset];
    }

    Layer operator[](IntType index) { ENG_MUTABLE_VERSION(operator[], index); }
    const Layer operator[](IntType index) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(debug::boundsCheck(index, m_Bounds.min.i, m_Bounds.max.i + 1), "Index is out of bounds!");
      IBox2<IntType> layerBounds(m_Bounds.min.j, m_Bounds.min.k, m_Bounds.max.j, m_Bounds.max.k);
      return Layer(m_Data.get() + m_Strides.i * (index - m_Bounds.min.i), layerBounds);
    }

    uSize size() const { return m_Bounds.volume(); }
    const IBox3<IntType>& bounds() const { return m_Bounds; }

    bool contains(const T& value) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return std::any_of(m_Data.get(), m_Data.get() + size(), [&value](const T& data)
      {
        return data == value;
      });
    }

    bool filledWith(const T& value) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return std::all_of(m_Data.get(), m_Data.get() + size(), [&value](const T& data)
      {
        return data == value;
      });
    }

    bool contentsEqual(const IBox3<IntType>& compareSection, const ArrayBox<T, IntType>& container, const IBox3<IntType>& containerSection, const T& defaultValue) const
    {
      ENG_CORE_ASSERT(compareSection.extents() == containerSection.extents(), "Compared sections are not the same dimensions!");

      if (!m_Data && !container)
        return true;
      if (!m_Data)
        return container.allOf(containerSection, [&defaultValue](const T& value) { return value == defaultValue; });
      if (!container)
        return allOf(compareSection, [&defaultValue](const T& value) { return value == defaultValue; });

      IVec3<IntType> offset = containerSection.min - compareSection.min;
      return allOf(compareSection, [&container, &offset](const IVec3<IntType>& index, const T& data)
      {
        return data == container(index + offset);
      });
    }



    template<std::predicate<const T&> F>
    bool allOf(F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return std::all_of(m_Data.get(), m_Data.get() + size(), std::forward<F>(condition));
    }

    template<std::predicate<const T&> F>
    bool allOf(const IBox3<IntType>& section, F&& condition) const { return allOf(section, toIndexed(std::forward<F>(condition))); }

    template<std::predicate<const IVec3<IntType>&, const T&> F>
    bool allOf(F&& condition) const { return allOf(m_Bounds, std::forward<F>(condition)); }

    template<std::predicate<const IVec3<IntType>&, const T&> F>
    bool allOf(const IBox3<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::allOf(section, [this, &condition](const IVec3<IntType>& index) { return condition(index, (*this)(index)); });
    }



    template<std::predicate<const T&> F>
    bool anyOf(F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return std::any_of(m_Data.get(), m_Data.get() + size(), std::forward<F>(condition));
    }

    template<std::predicate<const T&> F>
    bool anyOf(const IBox3<IntType>& section, F&& condition) const { return anyOf(section, toIndexed(std::forward<F>(condition))); }

    template<std::predicate<const IVec3<IntType>&, const T&> F>
    bool anyOf(F&& condition) const { return anyOf(m_Bounds, std::forward<F>(condition)); }

    template<std::predicate<const IVec3<IntType>&, const T&> F>
    bool anyOf(const IBox3<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::anyOf(section, [this, &condition](const IVec3<IntType>& index) { return condition(index, (*this)(index)); });
    }



    template<std::predicate<const T&> F>
    bool noneOf(F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return std::none_of(m_Data.get(), m_Data.get() + size(), std::forward<F>(condition));
    }

    template<std::predicate<const T&> F>
    bool noneOf(const IBox3<IntType>& section, F&& condition) const { return noneOf(section, toIndexed(std::forward<F>(condition))); }

    template<std::predicate<const IVec3<IntType>&, const T&> F>
    bool noneOf(F&& condition) const { return noneOf(m_Bounds, std::forward<F>(condition)); }

    template<std::predicate<const IVec3<IntType>&, const T&> F>
    bool noneOf(const IBox3<IntType>& section, F&& condition) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      return algo::noneOf(section, [this, &condition](const IVec3<IntType>& index) { return condition(index, (*this)(index)); });
    }



    void fill(const T& value)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      std::fill_n(m_Data.get(), size(), value);
    }

    void fill(const IBox3<IntType>& fillSection, const T& value)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      forEach(fillSection, [&value](T& data) { data = value; });
    }

    void fill(const IBox3<IntType>& fillSection, const ArrayBox<T, IntType>& container, const IBox3<IntType>& containerSection, const T& defaultValue)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      ENG_CORE_ASSERT(fillSection.extents() == containerSection.extents(), "Read and write sections are not the same dimensions!");

      if (!container)
      {
        fill(fillSection, defaultValue);
        return;
      }

      IVec3<IntType> offset = containerSection.min - fillSection.min;
      forEach(fillSection, [&container, &offset](const IVec3<IntType>& index, T& data) { data = container(index + offset); });
    }



    template<std::invocable<const T&> F>
    void forEach(F&& function) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      std::for_each_n(m_Data.get(), size(), std::forward<F>(function));
    }

    template<std::invocable<const T&> F>
    void forEach(const IBox3<IntType>& section, F&& function) const { forEach(section, toIndexed(function)); }

    template<std::invocable<const IVec3<IntType>&, const T&> F>
    void forEach(F&& function) const { forEach(m_Bounds, std::forward<F>(function)); }

    template<std::invocable<const IVec3<IntType>&, const T&> F>
    void forEach(const IBox3<IntType>& section, F&& function) const
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      algo::forEach(section, [this, &function](const IVec3<IntType>& index) { function(index, (*this)(index)); });
    }



    template<std::invocable<T&> F>
    void forEach(F&& function)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      std::for_each_n(m_Data.get(), size(), std::forward<F>(function));
    }

    template<std::invocable<T&> F>
    void forEach(const IBox3<IntType>& section, F&& function) { forEach(section, toIndexed(function)); }

    template<std::invocable<const IVec3<IntType>&, T&> F>
    void forEach(F&& function) { forEach(m_Bounds, std::forward<F>(function)); }

    template<std::invocable<const IVec3<IntType>&, T&> F>
    void forEach(const IBox3<IntType>& section, F&& function)
    {
      ENG_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
      algo::forEach(section, [this, &function](const IVec3<IntType>& index) { function(index, (*this)(index)); });
    }



    void setBounds(const IBox3<IntType>& bounds)
    {
      m_Bounds = bounds;
      IVec3<iSize> extents = m_Bounds.extents().upcast<iSize>();
      m_Strides = IVec2<iSize>(extents.j * extents.k, extents.k);
      m_Offset = m_Strides.i * m_Bounds.min.i + m_Strides.j * m_Bounds.min.j + m_Bounds.min.k;
    }

    void allocate()
    {
      if (m_Data)
        ENG_CORE_WARN("Data already allocated to ArrayBox. Ignoring...");
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
      return [this, &function](const IVec3<IntType>& /*unused*/, T& data) { return function(data); };
    }

    template<std::invocable<const T&> F>
    auto toIndexed(F&& function) const
    {
      return [this, &function](const IVec3<IntType>& /*unused*/, const T& data) { return function(data); };
    }
  };
}