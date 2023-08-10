#pragma once

template<typename T, int Expansion, int Size>
class ExpandedArraySection
{
public:
  ExpandedArraySection(T* begin)
    : m_Begin(begin) {}

  T& operator[](int index)
  {
    return const_cast<T&>(static_cast<const ExpandedArraySection*>(this)->operator[](index));
  }
  const T& operator[](int index) const
  {
    EN_ASSERT(boundsCheck(index, -Expansion, Size + Expansion), "Index is out of bounds!");
    return m_Begin[index + Expansion];
  }

private:
  T* m_Begin;
};

template<typename T, int Expansion, int Columns, int Depth = Columns>
class ExpandedArraySection2D
{
public:
  ExpandedArraySection2D(T* begin)
    : m_Begin(begin) {}

  ExpandedArraySection<T, Expansion, Depth> operator[](int columnIndex)
  {
    return static_cast<const ExpandedArraySection2D&>(*this).operator[](columnIndex);
  }
  const ExpandedArraySection<T, Expansion, Depth> operator[](int columnIndex) const
  {
    EN_ASSERT(boundsCheck(columnIndex, -Expansion, Columns + Expansion), "Index is out of bounds!");
    return ExpandedArraySection<T, Expansion, Depth>(m_Begin + Depth * (columnIndex + Expansion));
  }

private:
  T* m_Begin;
};



template<typename T, int Expansion, int Rows, int Columns = Rows>
class ExpandedArray2D
{
public:
  ExpandedArray2D() = default;
  ~ExpandedArray2D() { delete[] m_Data; }

  ExpandedArray2D(const ExpandedArray2D& other) = delete;
  ExpandedArray2D& operator=(const ExpandedArray2D& other) = delete;

  ExpandedArray2D(ExpandedArray2D&& other) noexcept
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  ExpandedArray2D& operator=(ExpandedArray2D&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  ExpandedArraySection<T, Expansion, Columns> operator[](int rowIndex)
  {
    return static_cast<const ExpandedArray2D&>(*this).operator[](rowIndex);
  }
  const ExpandedArraySection<T, Expansion, Columns> operator[](int rowIndex) const
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(boundsCheck(rowIndex, -Expansion, Rows + Expansion), "Index is out of bounds!");
    return ExpandedArraySection<T, Expansion, Columns>(m_Data + Columns * (rowIndex + Expansion));
  }

  operator bool() const { return m_Data; }

  constexpr int size() { return (Rows + Expansion) * (Columns + Expansion); }

  const T* get() const { return m_Data; }

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

  friend ExpandedArray2D AllocateExpandedArray2D<T, Expansion, Rows, Columns>();
  friend ExpandedArray2D AllocateExpandedArray2D<T, Expansion, Rows, Columns>(const T& initialValue);
};

template<typename T, int Expansion, int Rows, int Columns = Rows>
ExpandedArray2D<T, Expansion, Rows, Columns> AllocateExpandedArray2D()
{
  ExpandedArray2D<T, Expansion, Rows, Columns> arr;
  arr.allocate();
  return arr;
}

template<typename T, int Expansion, int Rows, int Columns = Rows>
ExpandedArray2D<T, Expansion, Rows, Columns> AllocateExpandedArray2D(const T& initialValue)
{
  ExpandedArray2D<T, Expansion, Rows, Columns> arr = AllocateExpandedArray2D<T, Expansion, Rows, Columns>();
  arr.fill(initialValue);
  return arr;
}



template<typename T, int Expansion, int Rows, int Columns = Rows, int Depth = Columns>
class ExpandedArray3D
{
public:
  ExpandedArray3D() = default;
  ~ExpandedArray3D() { delete[] m_Data; }

  ExpandedArray3D(const ExpandedArray3D& other) = delete;
  ExpandedArray3D& operator=(const ExpandedArray3D& other) = delete;

  ExpandedArray3D(ExpandedArray3D&& other) noexcept
  {
    m_Data = other.m_Data;
    other.m_Data = nullptr;
  }

  ExpandedArray3D& operator=(ExpandedArray3D&& other) noexcept
  {
    if (&other != this)
    {
      delete[] m_Data;
      m_Data = other.m_Data;
      other.m_Data = nullptr;
    }
    return *this;
  }

  ExpandedArraySection2D<T, Expansion, Columns, Depth> operator[](int rowIndex)
  {
    return static_cast<const ExpandedArray3D&>(*this).operator[](rowIndex);
  }
  const ExpandedArraySection2D<T, Expansion, Columns, Depth> operator[](int rowIndex) const
  {
    EN_ASSERT(m_Data, "Data has not yet been allocated!");
    EN_ASSERT(boundsCheck(rowIndex, -Expansion, Rows + Expansion), "Index is out of bounds!");
    return ExpandedArraySection2D<T, Expansion, Columns, Depth>(m_Data + Columns * Depth * (rowIndex + Expansion));
  }

  operator bool() const { return m_Data; }

  constexpr int size() const { return (Rows + Expansion) * (Columns + Expansion) * (Depth + Expansion); }

  const T* get() const { return m_Data; }

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

  friend ExpandedArray3D AllocateExpandedArray3D<T, Expansion, Rows, Columns, Depth>();
  friend ExpandedArray3D AllocateExpandedArray3D<T, Expansion, Rows, Columns, Depth>(const T& initialValue);
};

template<typename T, int Expansion, int Rows, int Columns = Rows, int Depth = Columns>
ExpandedArray3D<T, Expansion, Rows, Columns, Depth> AllocateExpandedArray3D()
{
  ExpandedArray3D<T, Expansion, Rows, Columns, Depth> arr;
  arr.allocate();
  return arr;
}

template<typename T, int Expansion, int Rows, int Columns = Rows, int Depth = Columns>
ExpandedArray3D<T, Expansion, Rows, Columns, Depth> AllocateExpandedArray3D(const T& initialValue)
{
  ExpandedArray3D<T, Expansion, Rows, Columns, Depth> arr = AllocateExpandedArray3D<T, Expansion, Rows, Columns, Depth>();
  arr.fill(initialValue);
  return arr;
}