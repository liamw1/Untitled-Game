#pragma once
#include "ArrayBox.h"

template<typename T, int MinX, int MaxX, int MinY = MinX, int MaxY = MaxX>
class ArrayRect : private Engine::NonCopyable
{
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

  template<std::integral IndexType>
  T& operator()(const IVec2<IndexType>& index)
  {
    return const_cast<T&>(static_cast<const ArrayBox*>(this)->operator()(index));
  }

  template<std::integral IndexType>
  const T& operator()(const IVec2<IndexType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    static constexpr IVec2<IndexType> base = IVec2<IndexType>(MinX, MinY);
    IVec2<IndexType> indexRelativeToBase = index - base;

    return m_Data[c_Stride * indexRelativeToBase.i + indexRelativeToBase.j];
  }

  ArrayBoxStrip<T, MinY, MaxY> operator[](int index)
  {
    return static_cast<const ArrayRect&>(*this).operator[](index);
  }
  const ArrayBoxStrip<T, MinY, MaxY> operator[](int index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(boundsCheck(index, MinX, MaxX), "Index is out of bounds!");
    return ArrayBoxStrip<T, MinY, MaxY>(m_Data + c_Stride * (index - MinX));
  }

  constexpr size_t size() const { return static_cast<size_t>(MaxX - MinX) * c_Stride; }

  void fill(const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), value);
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  T* m_Data;

  static constexpr size_t c_Stride = MaxY - MinY;
};