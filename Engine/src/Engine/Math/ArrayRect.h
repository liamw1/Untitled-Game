#pragma once
#include "ArrayBox.h"
#include "IBox2.h"

template<typename T, std::integral IntType>
class ArrayRect : private Engine::NonCopyable
{
public:
  using Strip = ArrayBoxStrip<T, IntType>;

public:
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

  T& operator()(const IVec2<IntType>& index)
  {
    return const_cast<T&>(static_cast<const ArrayRect*>(this)->operator()(index));
  }

  const T& operator()(const IVec2<IntType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(m_Bounds.encloses(index), "Index is out of bounds!");
    return m_Data[m_Stride * index.i + index.j - m_Offset];
  }

  Strip operator[](IntType index)
  {
    return static_cast<const ArrayRect&>(*this).operator[](index);
  }
  const Strip operator[](IntType index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, m_Bounds.min.i, m_Bounds.max.i), "Index is out of bounds!");
    return Strip(m_Data + m_Stride * (index - m_Bounds.min.i), m_Bounds.min.j);
  }

  size_t size() const { return m_Bounds.volume(); }

  const IBox2<IntType>& bounds() const { return m_Bounds; }

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

  bool contentsEqual(const IBox2<IntType>& compareSection, const ArrayBox<T, IntType>& container, const IBox2<IntType>& containerSection, const T& defaultValue) const
  {
    EN_CORE_ASSERT(compareSection.extents() == containerSection.extents(), "Compared sections are not the same dimensions!");

    if (!m_Data && !container)
      return true;
    if (!m_Data)
      return container.allOf(containerSection, [&defaultValue](const T& value) { return value == defaultValue; });
    if (!container)
      return this->allOf(compareSection, [&defaultValue](const T& value) { return value == defaultValue; });

    IVec2<IntType> offset = containerSection.min - compareSection.min;
    return compareSection.allOf([this, &container, &offset](const IVec2<IntType>& index)
      {
        return (*this)(index) == container(index + offset);
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool allOf(const IBox2<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.allOf([this, &condition](const IVec2<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool anyOf(const IBox2<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.anyOf([this, &condition](const IVec2<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<InvocableWithReturnType<bool, T> F>
  bool noneOf(const IBox2<IntType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.noneOf([this, &condition](const IVec2<IntType>& index)
      {
        return condition((*this)(index));
      });
  }

  void fill(const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), value);
  }

  void fill(const IBox2<IntType>& fillSection, const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &value](const IVec2<IntType>& index)
      {
        (*this)(index) = value;
      });
  }

  void fill(const IBox2<IntType>& fillSection, const ArrayBox<T, IntType>& container, const IBox2<IntType>& containerSection)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(fillSection.extents() == containerSection.extents(), "Read and write sections are not the same dimensions!");

    IVec2<IntType> offset = containerSection.min - fillSection.min;
    fillSection.forEach([this, &container, &offset](const IVec2<IntType>& index)
      {
        (*this)(index) = container(index + offset);
      });
  }

  void setBounds(const IBox2<IntType>& bounds)
  {
    m_Bounds = bounds;
    m_Stride = m_Bounds.max.j - m_Bounds.min.j;
    m_Offset = m_Stride * m_Bounds.min.i + m_Bounds.min.j;
  }

  void allocate()
  {
    if (m_Data)
      EN_CORE_WARN("Data already allocated to ArrayRect. Ignoring...");
    else
      m_Data = new T[size()];
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  IBox2<IntType> m_Bounds;
  int m_Stride;
  int m_Offset;
  T* m_Data;
};