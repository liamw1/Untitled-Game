#pragma once
#include "MultiDimArrays.h"

template<typename T, int Len, int Base>
  requires positive<Len>
class CubicArraySection
{
public:
  CubicArraySection(T* begin)
    : m_Begin(begin) {}

  ArraySection<T, Len, Base> operator[](int columnIndex)
  {
    return static_cast<const CubicArraySection*>(this)->operator[](columnIndex);
  }
  const ArraySection<T, Len, Base> operator[](int columnIndex) const
  {
    EN_CORE_ASSERT(boundsCheck(columnIndex - Base, 0, Len), "Index is out of bounds!");
    return ArraySection<T, Len, Base>(m_Begin + Len * (columnIndex - Base));
  }

private:
  T* m_Begin;

  ~CubicArraySection() = default;
};



template<typename T, int Len, int Base = 0>
  requires positive<Len>
class SquareArray
{
public:
  SquareArray()
    : m_Data(nullptr) {}
  SquareArray(AllocationPolicy policy)
    : m_Data(new T[size()]) { }
  SquareArray(const T& initialValue)
    : SquareArray(AllocationPolicy::ForOverWrite) { fill(initialValue); }
  ~SquareArray() { delete[] m_Data; }

  SquareArray(SquareArray&& other) noexcept
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  SquareArray& operator=(SquareArray&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  ArraySection<T, Len, Base> operator[](int rowIndex)
  {
    return static_cast<const SquareArray*>(this)->operator[](rowIndex);
  }
  const ArraySection<T, Len, Base> operator[](int rowIndex) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(boundsCheck(rowIndex - Base, 0, Len), "Index is out of bounds!");
    return ArraySection<T, Len, Base>(m_Data + Len * (rowIndex - Base));
  }

  operator bool() const { return m_Data; }

  constexpr int size() const { return Len * Len; }

  void fill(const T& val)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), val);
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  T* m_Data;

  SquareArray(const SquareArray& other) = delete;
  SquareArray& operator=(const SquareArray& other) = delete;
};



template<typename T, int Len, int Base = 0>
  requires positive<Len>
class CubicArray
{
public:
  CubicArray()
    : m_Data(nullptr) {}
  CubicArray(AllocationPolicy policy)
    : m_Data(new T[size()]) { }
  CubicArray(const T& initialValue)
    : CubicArray(AllocationPolicy::ForOverWrite) { fill(initialValue); }
  ~CubicArray() { delete[] m_Data; }

  CubicArray(CubicArray&& other) noexcept
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  CubicArray& operator=(CubicArray&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  CubicArraySection<T, Len, Base> operator[](int rowIndex)
  {
    return static_cast<const CubicArray*>(this)->operator[](rowIndex);
  }
  const CubicArraySection<T, Len, Base> operator[](int rowIndex) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(boundsCheck(rowIndex - Base, 0, Len), "Index is out of bounds!");
    return CubicArraySection<T, Len, Base>(m_Data + Len * Len * (rowIndex - Base));
  }

  operator bool() const { return m_Data; }

  template<std::integral IndexType>
  T& operator()(const IVec3<IndexType>& index)
  {
    return const_cast<T&>(static_cast<const CubicArray*>(this)->operator()(index));
  }

  template<std::integral IndexType>
  const T& operator()(const IVec3<IndexType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return m_Data[Len * Len * (index.i - Base) + Len * (index.j - Base) + (index.k - Base)];
  }

  constexpr int size() const { return Len * Len * Len; }

  bool filledWith(const T& val) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return std::all_of(m_Data, m_Data + size(), [&val](const T& data)
      {
        return data == val;
      });
  }

  template<std::integral IndexType>
  bool filledWith(const IBox3<IndexType>& section, const T& val) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.allOf([this, &val](const IVec3<IndexType>& index)
      {
        return (*this)(index) == val;
      });
  }

  template<std::integral IndexType, int ArrLen, int ArrBase>
  bool contentsEqual(const IBox3<IndexType>& compareSection, const CubicArray<T, ArrLen, ArrBase>& arr, const IVec3<IndexType>& arrBase)
  {
    if (!m_Data && !arr)
      return true;
    if (!m_Data || !arr)
      return false;

    return compareSection.allOf([this, &compareSection, &arr, &arrBase](const IVec3<IndexType>& index)
      {
        return (*this)(index) == arr(arrBase + index - compareSection.min);
      });
  }

  void fill(const T& val)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), val);
  }

  template<std::integral IndexType>
  void fill(const IBox3<IndexType>& fillSection, const T& val)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &val](const IVec3<IndexType>& index)
      {
        (*this)(index) = val;
      });
  }

  template<std::integral IndexType, int ArrLen, int ArrBase>
  void fill(const IBox3<IndexType>& fillSection, const CubicArray<T, ArrLen, ArrBase>& arr, const IVec3<IndexType>& arrBase)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &fillSection, &arr, &arrBase](const IVec3<IndexType>& index)
      {
        (*this)(index) = arr(arrBase + index - fillSection.min);
      });
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

  template<std::integral IndexType>
  static constexpr IBox3<IndexType> Bounds() { return IBox3<IndexType>(Base, Base + Len); }

private:
  T* m_Data;

  CubicArray(const CubicArray& other) = delete;
  CubicArray& operator=(const CubicArray& other) = delete;
};