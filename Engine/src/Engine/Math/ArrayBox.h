#pragma once
#include "IBox2.h"
#include "IBox3.h"
#include "Engine/Utilities/Constraints.h"

enum class AllocationPolicy
{
  Deferred,
  ForOverwrite,
  DefaultInitialize
};

template<typename T, std::integral IntType>
class ArrayBoxStrip
{
public:
  ArrayBoxStrip(T* begin, int offset)
    : m_Begin(begin), m_Offset(offset) {}

  T& operator[](IntType index)
  {
    return const_cast<T&>(static_cast<const ArrayBoxStrip*>(this)->operator[](index));
  }
  const T& operator[](IntType index) const
  {
    return m_Begin[index - m_Offset];
  }

private:
  T* m_Begin;
  int m_Offset;

  ~ArrayBoxStrip() = default;
};

template<typename T, std::integral IntType>
class ArrayBoxLayer
{
public:
  ArrayBoxLayer(T* begin, const IBox2<IntType>& bounds)
    : m_Begin(begin), m_Bounds(bounds) {}

  ArrayBoxStrip<T, IntType> operator[](IntType index)
  {
    return static_cast<const ArrayBoxLayer&>(*this).operator[](index);
  }
  const ArrayBoxStrip<T, IntType> operator[](IntType index) const
  {
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, m_Bounds.min.i, m_Bounds.max.i + 1), "Index is out of bounds!");
    return ArrayBoxStrip<T, IntType>(m_Begin + m_Bounds.extents().j * (index - m_Bounds.min.i), m_Bounds.min.j);
  }

private:
  T* m_Begin;
  IBox2<IntType> m_Bounds;

  ~ArrayBoxLayer() = default;
};



/*
  A 3D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed with brackets:
  arr[i][j][k]
*/
template<typename T, std::integral IntType>
class ArrayBox : private Engine::NonCopyable
{
public:
  using Layer = ArrayBoxLayer<T, IntType>;
  using Strip = ArrayBoxStrip<T, IntType>;

public:
  ArrayBox(const IBox3<IntType>& bounds, AllocationPolicy policy)
    : m_Data(nullptr)
  {
    setBounds(bounds);
    if (policy != AllocationPolicy::Deferred)
      allocate();
    if (policy == AllocationPolicy::DefaultInitialize)
      fill(T());
  }
  ArrayBox(const IBox3<IntType>& bounds, const T& initialValue)
    : ArrayBox(bounds, AllocationPolicy::ForOverwrite) { fill(initialValue); }
  ~ArrayBox() { delete[] m_Data; }

  ArrayBox(ArrayBox&& other) noexcept = default;
  ArrayBox& operator=(ArrayBox&& other) noexcept
  {
    if (&other != this)
    {
      m_Bounds = other.m_Bounds;
      m_Strides = other.m_Strides;
      m_Offset = other.m_Offset;

      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  operator bool() const { return m_Data; }

  T& operator()(const IVec3<IntType>& index)
  {
    return const_cast<T&>(static_cast<const ArrayBox*>(this)->operator()(index));
  }

  const T& operator()(const IVec3<IntType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(m_Bounds.encloses(index), "Index is out of bounds!");
    return m_Data[m_Strides.i * index.i + m_Strides.j * index.j + index.k - m_Offset];
  }

  Layer operator[](IntType index)
  {
    return static_cast<const ArrayBox*>(this)->operator[](index);
  }
  const Layer operator[](IntType index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, m_Bounds.min.i, m_Bounds.max.i + 1), "Index is out of bounds!");
    IBox2<IntType> layerBounds(m_Bounds.min.j, m_Bounds.min.k, m_Bounds.max.j, m_Bounds.max.k);
    return Layer(m_Data + m_Strides.i * (index - m_Bounds.min.i), layerBounds);
  }

  size_t size() const { return m_Bounds.volume(); }
  const IBox3<IntType>& bounds() const { return m_Bounds; }

  bool contains(const T& value) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return std::any_of(m_Data, m_Data + size(), [&value](const T& data)
      {
        return data == value;
      });
  }

  bool filledWith(const T& value) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return std::all_of(m_Data, m_Data + size(), [&value](const T& data)
      {
        return data == value;
      });
  }

  bool contentsEqual(const IBox3<IntType>& compareSection, const ArrayBox<T, IntType>& container, const IBox3<IntType>& containerSection, const T& defaultValue) const
  {
    EN_CORE_ASSERT(compareSection.extents() == containerSection.extents(), "Compared sections are not the same dimensions!");

    if (!m_Data && !container)
      return true;
    if (!m_Data)
      return container.allOf(containerSection, [&defaultValue](const T& value) { return value == defaultValue; });
    if (!container)
      return this->allOf(compareSection, [&defaultValue](const T& value) { return value == defaultValue; });

    IVec3<IntType> offset = containerSection.min - compareSection.min;
    return compareSection.allOf([this, &container, &offset](const IVec3<IntType>& index)
      {
        return (*this)(index) == container(index + offset);
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool allOf(const IBox3<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.allOf([this, &condition](const IVec3<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool anyOf(const IBox3<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.anyOf([this, &condition](const IVec3<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool noneOf(const IBox3<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.noneOf([this, &condition](const IVec3<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  void fill(const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), value);
  }

  void fill(const IBox3<IntType>& fillSection, const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &value](const IVec3<IntType>& index)
      {
        (*this)(index) = value;
      });
  }

  void fill(const IBox3<IntType>& fillSection, const ArrayBox<T, IntType>& container, const IBox3<IntType>& containerSection, const T& defaultValue)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(fillSection.extents() == containerSection.extents(), "Read and write sections are not the same dimensions!");

    if (!container)
    {
      fill(fillSection, defaultValue);
      return;
    }

    IVec3<IntType> offset = containerSection.min - fillSection.min;
    fillSection.forEach([this, &container, &offset](const IVec3<IntType>& index)
      {
        (*this)(index) = container(index + offset);
      });
  }

  void setBounds(const IBox3<IntType>& bounds)
  {
    m_Bounds = bounds;
    IVec3<int> extents = static_cast<IVec3<int>>(m_Bounds.extents());
    m_Strides = IVec2<int>(extents.j * extents.k, extents.k);
    m_Offset = m_Strides.i * m_Bounds.min.i + m_Strides.j * m_Bounds.min.j + m_Bounds.min.k;
  }

  void allocate()
  {
    if (m_Data)
      EN_CORE_WARN("Data already allocated to ArrayBox. Ignoring...");
    else
      m_Data = new T[size()];
  }

  void clear()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  IBox3<IntType> m_Bounds;
  IVec2<int> m_Strides;
  int m_Offset;
  T* m_Data;
};