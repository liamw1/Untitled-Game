#pragma once
#include "ArrayBox.h"
#include "IBox2.h"

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
    return const_cast<T&>(static_cast<const ArrayRect*>(this)->operator()(index));
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
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, MinX, MaxX), "Index is out of bounds!");
    return ArrayBoxStrip<T, MinY, MaxY>(m_Data + c_Stride * (index - MinX));
  }

  T* get() const { return m_Data; }

  constexpr size_t size() const { return static_cast<size_t>(MaxX - MinX) * c_Stride; }

  template<std::signed_integral IndexType>
  constexpr IBox2<IndexType> box() const { return static_cast<IBox2<IndexType>>(c_Box); }

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

  template<std::integral IndexType, int... Args>
  bool contentsEqual(const IBox2<IndexType>& compareSection, const ArrayRect<T, Args...>& arr, const IBox2<IndexType>& arrSection) const
  {
    if (!m_Data && !arr)
      return true;
    if (!m_Data || !arr)
      return false;

    IVec2<IndexType> offset = arrSection.min - compareSection.min;
    return compareSection.allOf([this, &arr, &offset](const IVec2<IndexType>& index)
      {
        return (*this)(index) == arr(index + offset);
      });
  }

  template<std::integral IndexType, InvocableWithReturnType<bool, T> F>
  bool allOf(const IBox2<IndexType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.allOf([this, &condition](const IVec2<IndexType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<std::integral IndexType, InvocableWithReturnType<bool, T> F>
  bool anyOf(const IBox2<IndexType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.anyOf([this, &condition](const IVec2<IndexType>& index)
      {
        return condition((*this)(index));
      });
  }

  template<std::integral IndexType, InvocableWithReturnType<bool, T> F>
  bool noneOf(const IBox2<IndexType>& section, const F& condition) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.noneOf([this, &condition](const IVec2<IndexType>& index)
      {
        return condition((*this)(index));
      });
  }

  void fill(const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), value);
  }

  template<std::integral IndexType>
  void fill(const IBox2<IndexType>& fillSection, const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &value](const IVec2<IndexType>& index)
      {
        (*this)(index) = value;
      });
  }

  template<std::integral IndexType, int... Args>
  void fill(const IBox2<IndexType>& fillSection, const ArrayRect<T, Args...>& arr, const IBox2<IndexType>& arrSection)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    IVec2<IndexType> offset = arrSection.min - fillSection.min;
    fillSection.forEach([this, &arr, &offset](const IVec2<IndexType>& index)
      {
        (*this)(index) = arr(index + offset);
      });
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  static constexpr IBox2<int> c_Box = IBox2<int>(MinX, MinY, MaxX, MaxY);
  static constexpr size_t c_Stride = MaxY - MinY;

  T* m_Data;
};