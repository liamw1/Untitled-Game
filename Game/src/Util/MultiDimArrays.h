#pragma once

/*
  A 2D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed like:
  arr(i, j)

  Huge advantages in efficiency for "tall" arrays (rows >> cols).
*/
template<typename T, int Rows, int Columns = Rows>
class Array2D
{
public:
  Array2D() = default;
  ~Array2D() { delete[] m_Data; }

  Array2D(const Array2D& other) = delete;
  Array2D& operator=(const Array2D& other) = delete;

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

  T& operator()(int rowIndex, int columnIndex)
  {
    return const_cast<T&>(static_cast<const Array2D&>(*this).operator()(rowIndex, columnIndex));
  }

  const T& operator()(int rowIndex, int columnIndex) const
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(boundsCheck(rowIndex, 0, Rows) && boundsCheck(columnIndex, 0, Columns), "Index is out of bounds!");
    return m_Data[Columns * rowIndex + columnIndex];
  }

  T& operator[](int index)
  {
    return const_cast<T&>(static_cast<const Array2D&>(*this).operator[](index));
  }

  const T& operator[](int index) const
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(boundsCheck(index, 0, size()), "Index is out of bounds!");
    return m_Data[index];
  }

  operator bool() const { return m_Data; }

  constexpr int size() { return Rows * Columns; }

  void fill(const T& val)
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), val);
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  T* m_Data = nullptr;

  void allocate() { m_Data = new T[size()]; }

  friend Array2D AllocateArray2D<T, Rows, Columns>();
  friend Array2D AllocateArray2D<T, Rows, Columns>(const T& initialValue);
};

template<typename T, int Rows, int Columns = Rows>
Array2D<T, Rows, Columns> AllocateArray2D()
{
  Array2D<T, Rows, Columns> arr;
  arr.allocate();
  return arr;
}

template<typename T, int Rows, int Columns = Rows>
Array2D<T, Rows, Columns> AllocateArray2D(const T& initialValue)
{
  Array2D<T, Rows, Columns> arr = AllocateArray2D<T, Rows, Columns>();
  arr.fill(initialValue);
  return arr;
}



/*
  A 3D-style array that stores its members in a single
  heap-allocated block in memory.

  Elements can be accessed with brackets:
  arr(i, j, k)
*/
template<typename T, int Rows, int Columns = Rows, int Depth = Columns>
class Array3D
{
public:
  Array3D() = default;
  ~Array3D() { delete[] m_Data; }

  Array3D(const Array3D& other) = delete;
  Array3D& operator=(const Array3D& other) = delete;

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

  T& operator()(int rowIndex, int columnIndex, int depthIndex)
  {
    return const_cast<T&>(static_cast<const Array3D&>(*this).operator()(rowIndex, columnIndex, depthIndex));
  }

  const T& operator()(int rowIndex, int columnIndex, int depthIndex) const
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(boundsCheck(rowIndex, 0, Rows) && boundsCheck(columnIndex, 0, Columns) && boundsCheck(depthIndex, 0, Depth), "Index is out of bounds!");
    return m_Data[Columns * Depth * rowIndex + Depth * columnIndex + depthIndex];
  }

  T& operator[](int index)
  {
    return const_cast<T&>(static_cast<const Array3D&>(*this).operator[](index));
  }

  const T& operator[](int index) const
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(boundsCheck(index, 0, size()), "Index is out of bounds!");
    return m_Data[index];
  }

  operator bool() const { return m_Data; }

  constexpr int size() const { return Rows * Columns * Depth; }

  void fill(const T& val)
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + size(), val);
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

private:
  T* m_Data = nullptr;

  void allocate() { m_Data = new T[size()]; }

  friend Array3D AllocateArray3D<T, Rows, Columns, Depth>();
  friend Array3D AllocateArray3D<T, Rows, Columns, Depth>(const T& initialValue);
};

template<typename T, int Rows, int Columns = Rows, int Depth = Columns>
Array3D<T, Rows, Columns, Depth> AllocateArray3D()
{
  Array3D<T, Rows, Columns, Depth> arr;
  arr.allocate();
  return arr;
}

template<typename T, int Rows, int Columns = Rows, int Depth = Columns>
Array3D<T, Rows, Columns, Depth> AllocateArray3D(const T& initialValue)
{
  Array3D<T, Rows, Columns, Depth> arr = AllocateArray3D<T, Rows, Columns, Depth>();
  arr.fill(initialValue);
  return arr;
}