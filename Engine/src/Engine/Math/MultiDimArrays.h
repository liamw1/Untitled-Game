#pragma once
#include "IBox.h"
#include "Engine/Utilities/Constraints.h"

enum class AllocationPolicy
{
  ForOverWrite
};

template<typename T, int Size, int Base = 0>
  requires positive<Size>
class ArraySection
{
public:
  ArraySection(T* begin)
    : m_Begin(begin) {}

  T& operator[](int index)
  {
    return const_cast<T&>(static_cast<const ArraySection*>(this)->operator[](index));
  }
  const T& operator[](int index) const
  {
    EN_CORE_ASSERT(boundsCheck(index - Base, 0, Size), "Index is out of bounds!");
    return m_Begin[index - Base];
  }

private:
  T* m_Begin;

  ~ArraySection() = default;
};



template<typename T, int Columns, int Depth = Columns>
  requires allPositive<Columns, Depth>
class ArraySection2D
{
public:
  ArraySection2D(T* begin)
    : m_Begin(begin) {}

  ArraySection<T, Depth> operator[](int columnIndex)
  {
    return static_cast<const ArraySection2D&>(*this).operator[](columnIndex);
  }
  const ArraySection<T, Depth> operator[](int columnIndex) const
  {
    EN_CORE_ASSERT(boundsCheck(columnIndex, 0, Columns), "Index is out of bounds!");
    return ArraySection<T, Depth>(m_Begin + Depth * columnIndex);
  }

private:
  T* m_Begin;

  ~ArraySection2D() = default;
};



/*
  A 2D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed like:
  arr[i][j]

  Huge advantages in efficiency for "tall" arrays (rows >> cols).
*/
template<typename T, int Rows, int Columns = Rows>
  requires allPositive<Rows, Columns>
class Array2D : private Engine::NonCopyable
{
public:
  Array2D()
    : m_Data(nullptr) {}
  Array2D(AllocationPolicy policy)
    : m_Data(new T[size()]) {}
  Array2D(const T& initialValue)
    : Array2D(AllocationPolicy::ForOverWrite) { fill(initialValue); }
  ~Array2D() { delete[] m_Data; }

  Array2D(Array2D&& other) noexcept
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  Array2D& operator=(Array2D&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  ArraySection<T, Columns> operator[](int rowIndex)
  {
    return static_cast<const Array2D&>(*this).operator[](rowIndex);
  }
  const ArraySection<T, Columns> operator[](int rowIndex) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(boundsCheck(rowIndex, 0, Rows), "Index is out of bounds!");
    return ArraySection<T, Columns>(m_Data + Columns * rowIndex);
  }

  operator bool() const { return m_Data; }

  constexpr int size() { return Rows * Columns; }

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
};



/*
  A 3D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed with brackets:
  arr[i][j][k]
*/
template<typename T, int Rows, int Columns = Rows, int Depth = Columns>
  requires allPositive<Rows, Columns, Depth>
class Array3D : private Engine::NonCopyable
{
public:
  Array3D()
    : m_Data(nullptr) {}
  Array3D(AllocationPolicy policy)
    : m_Data(new T[size()]) { }
  Array3D(const T& initialValue)
    : Array3D(AllocationPolicy::ForOverWrite) { fill(initialValue); }
  ~Array3D() { delete[] m_Data; }

  Array3D(Array3D&& other) noexcept
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  Array3D& operator=(Array3D&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  ArraySection2D<T, Columns, Depth> operator[](int rowIndex)
  {
    return static_cast<const Array3D&>(*this).operator[](rowIndex);
  }
  const ArraySection2D<T, Columns, Depth> operator[](int rowIndex) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_CORE_ASSERT(boundsCheck(rowIndex, 0, Rows), "Index is out of bounds!");
    return ArraySection2D<T, Columns, Depth>(m_Data + Columns * Depth * rowIndex);
  }

  operator bool() const { return m_Data; }

  constexpr int size() const { return Rows * Columns * Depth; }

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
};