#pragma once
#include "ArrayBox.h"
#include "IBox2.h"

template<typename T, std::integral IntType, IntType MinX, IntType MaxX, IntType MinY = MinX, IntType MaxY = MaxX>
class ArrayRect : private Engine::NonCopyable
{
public:
  using Strip = ArrayBoxStrip<T, IntType, MinY, MaxY>;

public:
  ArrayRect()
    : m_Data(nullptr) {}
  ArrayRect(AllocationPolicy policy)
    : m_Data(new T[size()]) {}
  ArrayRect(const T& initialValue)
    : ArrayRect(AllocationPolicy::ForOverWrite) { fill(initialValue); }
  ~ArrayRect() { delete[] m_Data; }

  ArrayRect(ArrayRect&& other) noexcept = default;
  ArrayRect& operator=(ArrayRect&& other) noexcept
  {
    if (&other != this)
    {
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

    IVec2<IntType> indexRelativeToBase = index - c_Bounds.min;
    return m_Data[c_Stride * indexRelativeToBase.i + indexRelativeToBase.j];
  }

  Strip operator[](IntType index)
  {
    return static_cast<const ArrayRect&>(*this).operator[](index);
  }
  const Strip operator[](IntType index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinX, MaxX), "Index is out of bounds!");
    return Strip(m_Data + c_Stride * (index - MinX));
  }

  T* get() const { return m_Data; }

  constexpr size_t size() const { return c_Bounds.volume(); }

  constexpr IBox2<IntType> bounds() const { return c_Bounds; }

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

  template<IntType... Args>
  bool contentsEqual(const IBox2<IntType>& compareSection, const ArrayBox<T, IntType, Args...>& container, const IBox2<IntType>& containerSection, const T& defaultValue) const
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

  template<IntType... Args>
  void fill(const IBox2<IntType>& fillSection, const ArrayBox<T, IntType, Args...>& container, const IBox2<IntType>& containerSection)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(fillSection.extents() == containerSection.extents(), "Read and write sections are not the same dimensions!");

    IVec2<IntType> offset = containerSection.min - fillSection.min;
    fillSection.forEach([this, &container, &offset](const IVec2<IntType>& index)
      {
        (*this)(index) = container(index + offset);
      });
  }

  void allocate()
  {
    if (m_Data)
    {
      EN_CORE_WARN("Data already allocated to ArrayRect. Ignoring...");
      return;
    }

    m_Data = new T[size()];
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  static constexpr IBox2<IntType> c_Bounds = IBox2<IntType>(MinX, MinY, MaxX, MaxY);
  static constexpr size_t c_Stride = MaxY - MinY;

  T* m_Data;
};