#pragma once

template<typename T, int N, int M = N>
using StackArray2D = std::array<std::array<T, M>, N>;



// Helper class that stores data for HeapArray2D
template<typename T, int N, int M = N>
class Container
{
public:
  Container()
    : m_Data(new T[N * M]) {}
  ~Container() { delete[] m_Data; }

  Container(const Container& other) = delete;
  Container& operator=(const Container& other) = delete;

  Container(Container&& other) noexcept
    : m_RowIndex(other.m_RowIndex)
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  Container& operator=(Container&& other) noexcept
  {
    if (&other != this)
    {
      m_RowIndex = other.m_RowIndex;

      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  T& operator[](int colIndex) 
  { 
    EN_ASSERT(colIndex < M, "Column index is out of bounds!");
    return m_Data[m_RowIndex * M + colIndex]; 
  }
  const T& operator[](int colIndex) const 
  { 
    EN_ASSERT(colIndex < M, "Column index is out of bounds!");
    return m_Data[m_RowIndex * M + colIndex]; 
  }

private:
  mutable int m_RowIndex = 0;
  T* m_Data = nullptr;

  template<typename U, int N, int M>
  friend class HeapArray2D;
};

/*
  A 2D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed with brackets:
  arr[i][j]

  Huge advantages in efficiency for "tall" arrays (rows >> cols).

  NOTE: Memory will be allocated upon construction but not initialized.
        Set elements before using their data to avoid undefined behaviour.
*/
template<typename T, int N, int M = N>
class HeapArray2D
{
public:
  HeapArray2D() = default;

  HeapArray2D(const HeapArray2D& other) = delete;
  HeapArray2D& operator=(const HeapArray2D& other) = delete;

  HeapArray2D(HeapArray2D&& other) noexcept
    : m_Container(std::move(other.m_Container)) {}

  HeapArray2D& operator=(HeapArray2D&& other) noexcept
  {
    if (&other != this)
      m_Container = std::move(other.m_Container);
    return *this;
  }

  [[nodiscard]]
  Container<T, N, M>& operator[](int rowIndex)
  {
    EN_ASSERT(rowIndex < N, "Row index is out of bounds!");
    m_Container.m_RowIndex = rowIndex;
    return m_Container;
  }

  [[nodiscard]]
  const Container<T, N, M>& operator[](int rowIndex) const
  {
    EN_ASSERT(rowIndex < N, "Row index is out of bounds!");
    m_Container.m_RowIndex = rowIndex;
    return m_Container;
  }

private:
  Container<T, N, M> m_Container;
};