#pragma once
#include "IBox.h"
#include "Engine/Utilities/Constraints.h"

enum class AllocationPolicy
{
  ForOverWrite
};

template<typename T, int MinZ, int MaxZ>
class ArrayBoxStrip
{
public:
  ArrayBoxStrip(T* begin)
    : m_Begin(begin) {}

  T& operator[](int index)
  {
    return const_cast<T&>(static_cast<const ArrayBoxStrip*>(this)->operator[](index));
  }
  const T& operator[](int index) const
  {
    EN_CORE_ASSERT(boundsCheck(index, MinZ, MaxZ), "Index is out of bounds!");
    return m_Begin[index - MinZ];
  }

private:
  T* m_Begin;

  ~ArrayBoxStrip() = default;
};

template<typename T, int MinY, int MaxY, int MinZ, int MaxZ>
class ArrayBoxLayer
{
public:
  ArrayBoxLayer(T* begin)
    : m_Begin(begin) {}

  ArrayBoxStrip<T, MinZ, MaxZ> operator[](int index)
  {
    return static_cast<const ArrayBoxLayer&>(*this).operator[](index);
  }
  const ArrayBoxStrip<T, MinZ, MaxZ> operator[](int index) const
  {
    EN_CORE_ASSERT(boundsCheck(index, MinY, MaxY), "Index is out of bounds!");
    return ArrayBoxStrip<T, MinZ, MaxZ>(m_Begin + (MaxZ - MinZ) * (index - MinY));
  }

private:
  T* m_Begin;

  ~ArrayBoxLayer() = default;
};



/*
  A 3D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed with brackets:
  arr[i][j][k]
*/
template<typename T, int MinX, int MaxX, int MinY = MinX, int MaxY = MaxX, int MinZ = MinX, int MaxZ = MaxX>
class ArrayBox : private Engine::NonCopyable
{
public:
  ArrayBox()
    : m_Data(nullptr) {}
  ArrayBox(AllocationPolicy policy)
    : m_Data(new T[size()]) {}
  ArrayBox(const T& initialValue)
    : ArrayBox(AllocationPolicy::ForOverWrite) { fill(initialValue); }
  ~ArrayBox() { delete[] m_Data; }

  ArrayBox(ArrayBox&& other) noexcept = default;
  ArrayBox& operator=(ArrayBox&& other) noexcept
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
  T& operator()(const IVec3<IndexType>& index)
  {
    return const_cast<T&>(static_cast<const ArrayBox*>(this)->operator()(index));
  }

  template<std::integral IndexType>
  const T& operator()(const IVec3<IndexType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    static constexpr IVec3<int> extents = c_Box.extents();
    static constexpr IVec3<size_t> strides(extents.j * extents.k, extents.k, 1);
    static constexpr IVec3<IndexType> base = static_cast<IVec3<IndexType>>(c_Box.min);

    IVec3<IndexType> indexRelativeToBase = index - base;
    return m_Data[strides.i * indexRelativeToBase.i + strides.j * indexRelativeToBase.j + strides.k * indexRelativeToBase.k];
  }

  ArrayBoxLayer<T, MinY, MaxY, MinZ, MaxZ> operator[](int index)
  {
    return static_cast<const ArrayBox&>(*this).operator[](index);
  }
  const ArrayBoxLayer<T, MinY, MaxY, MinZ, MaxZ> operator[](int index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(boundsCheck(index, MinX, MaxX), "Index is out of bounds!");

    static constexpr size_t stride = static_cast<size_t>(MaxY - MinY) * static_cast<size_t>(MaxZ - MinZ);
    return ArrayBoxLayer<T, MinY, MaxY, MinZ, MaxZ>(m_Data + stride * index);
  }

  constexpr size_t size() const { return c_Box.volume(); }

  template<std::signed_integral IndexType>
  constexpr IBox3<IndexType> box() const { return static_cast<IBox3<IndexType>>(c_Box); }

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

  template<std::integral IndexType>
  bool filledWith(const IBox3<IndexType>& section, const T& value) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return section.allOf([this, &value](const IVec3<IndexType>& index)
      {
        return (*this)(index) == value;
      });
  }

  template<std::integral IndexType, int... Args>
  bool contentsEqual(const IBox3<IndexType>& compareSection, const ArrayBox<T, Args...>& arr, const IVec3<IndexType>& arrBase)
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
    std::fill(m_Data, m_Data + size(), value);
  }

  template<std::integral IndexType>
  void fill(const IBox3<IndexType>& fillSection, const T& value)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    fillSection.forEach([this, &value](const IVec3<IndexType>& index)
      {
        (*this)(index) = value;
      });
  }

  template<std::integral IndexType, int... Args>
  void fill(const IBox3<IndexType>& fillSection, const ArrayBox<T, Args...>& arr, const IVec3<IndexType>& arrBase)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    IVec3<IndexType> offset = arrBase - fillSection.min;
    fillSection.forEach([this, &arr, offset](const IVec3<IndexType>& index)
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
  static constexpr IBox3<int> c_Box = IBox3<int>(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);

  T* m_Data;
};