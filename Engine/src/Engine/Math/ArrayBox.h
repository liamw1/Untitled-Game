#pragma once
#include "IBox.h"
#include "CubicArrays.h"

template<typename T, std::integral IndexType>
class ArrayBox
{
public:
  ArrayBox()
    : m_Box(IBox3<IndexType>::VoidBox()), m_Data(nullptr) {}
  ArrayBox(const IBox3<IndexType>& box)
    : m_Box(box), m_Data(new T[volume()]) {}
  ArrayBox(const IBox3<IndexType>& box, const T& initialValue)
    : ArrayBox(box) { fill(initialValue); }
  ~ArrayBox() { delete[] m_Data; }

  ArrayBox(ArrayBox&& other) noexcept
  {
    m_Box = std::move(other.m_Box);

    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  ArrayBox& operator=(ArrayBox&& other) noexcept
  {
    if (this != &other)
    {
      m_Box = std::move(other.m_Box);

      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  operator bool() const { return m_Data; }

  T& operator()(const IVec3<IndexType>& index)
  {
    return const_cast<T&>(static_cast<const ArrayBox*>(this)->operator()(index));
  }

  const T& operator()(const IVec3<IndexType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    IVec3<IndexType> extents = m_Box.extents();
    IVec3<int> extentsI(extents.i, extents.j, extents.k);
    int arrayIndex = extentsI.j * extentsI.k * static_cast<int>(index.i - m_Box.min.i) + extentsI.k * static_cast<int>(index.j - m_Box.min.j) + static_cast<int>(index.k - m_Box.min.k);
    EN_CORE_ASSERT(boundsCheck(arrayIndex, 0, volume()), "Index is out of bounds!");

    return m_Data[arrayIndex];
  }

  bool filledWith(const T& value) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return std::all_of(m_Data, m_Data + volume(), [&value](const T& data)
      {
        return data == value;
      });
  }

  bool filledWith(const IBox3<IndexType>& section, const T& value) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.allOf([this, &value](const IVec3<IndexType>& index)
      {
        return (*this)(index) == value;
      });
  }

  bool contentsEqual(const IBox3<IndexType>& compareSection, const ArrayBox<T, IndexType>& arr, const IVec3<IndexType>& arrBase) const
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

  void fill(const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + volume(), value);
  }

  void fill(const IBox3<IndexType>& fillSection, const T& val)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &val](const IVec3<IndexType>& index)
      {
        (*this)(index) = val;
      });
  }

  void fill(const IBox3<IndexType>& fillSection, const ArrayBox<T, IndexType>& arr, const IVec3<IndexType>& arrBase)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &fillSection, &arr, &arrBase](const IVec3<IndexType>& index)
      {
        (*this)(index) = arr(arrBase + index - fillSection.min);
      });
  }

  template<int ArrLen, int ArrBase>
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

  int volume() const
  {
    IVec3<IndexType> extents = m_Box.extents();
    return extents.i * extents.j * extents.k;
  }

private:
  IBox3<IndexType> m_Box;
  T* m_Data;

  ArrayBox(const ArrayBox& other) = delete;
  ArrayBox& operator=(const ArrayBox& other) = delete;
};