#pragma once

template<typename T, int N, int M = N>
using StackArray2D = std::array<std::array<T, M>, N>;

template<typename T, int N, int M = N, int D = M>
using StackArray3D = std::array<std::array<std::array<T, D>, M>, N>;

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
  ~HeapArray2D() { delete[] m_Data; }

  HeapArray2D(const HeapArray2D& other) = delete;
  HeapArray2D& operator=(const HeapArray2D& other) = delete;

  HeapArray2D(HeapArray2D&& other) noexcept
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  HeapArray2D& operator=(HeapArray2D&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  T& operator()(int index1, int index2)
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(index1 < N && index2 < M, "Index is out of bounds!");
    return m_Data[N * index1 + index2];
  }

  const T& operator()(int index1, int index2) const
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(index1 < N && index2 < M, "Index is out of bounds!");
    return m_Data[N * index1 + index2];
  }

  constexpr int size() { return N * M; }

private:
  T* m_Data = nullptr;

  void allocate() { m_Data = new T[size()]; }
  void fill(const T& initialValue) { std::fill(m_Data, m_Data + size(), initialValue); }

  friend HeapArray2D AllocateHeapArray2D<T, N, M>();
  friend HeapArray2D AllocateHeapArray2D<T, N, M>(const T& initialValue);
};

template<typename T, int N, int M = N>
HeapArray2D<T, N, M> AllocateHeapArray2D()
{
  HeapArray2D<T, N, M> arr;
  arr.allocate();
  return arr;
}

template<typename T, int N, int M = N>
HeapArray2D<T, N, M> AllocateHeapArray2D(const T& initialValue)
{
  HeapArray2D<T, N, M> arr = AllocateArray2D<T, N, M>();
  arr.fill(initialValue);
  return arr;
}