#pragma once
#include "IBox.h"

template<typename T, int Rows, int Columns = Rows>
using StackArray2D = std::array<std::array<T, Columns>, Rows>;

template<typename T, int Rows, int Columns = Rows, int Depth = Columns>
using StackArray3D = std::array<std::array<std::array<T, Depth>, Columns>, Rows>;



template<typename T, int Size, int Base = 0>
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

template<typename T, int Len, int Base>
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

template<typename T, int Columns, int Depth = Columns>
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
class Array2D
{
public:
  Array2D()
    : m_Data(nullptr) {}
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

  const T* get() const { return m_Data; }

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
  arr[i][j][k]
*/
template<typename T, int Rows, int Columns = Rows, int Depth = Columns>
class Array3D
{
public:
  Array3D()
    : m_Data(nullptr) {}
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

  const T* get() const { return m_Data; }

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



template<typename T, int Len, int Base = 0>
class SquareArray
{
public:
  SquareArray()
    : m_Data(nullptr)
  {
  }
  ~SquareArray() { delete[] m_Data; }

  SquareArray(const SquareArray& other) = delete;
  SquareArray& operator=(const SquareArray& other) = delete;

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

  const T* get() const { return m_Data; }

  void fill(const T& val)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + Size(), val);
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

  static constexpr int Length() { return Len; }
  static constexpr int Size() { return Len * Len; }

private:
  T* m_Data;

  void allocate() { m_Data = new T[Size()]; }

  friend SquareArray MakeSquareArray<T, Len, Base>();
  friend SquareArray MakeSquareArray<T, Len, Base>(const T& initialValue);
};

template<typename T, int Len, int Base = 0>
SquareArray<T, Len, Base> MakeSquareArray()
{
  SquareArray<T, Len, Base> arr;
  arr.allocate();
  return arr;
}

template<typename T, int Len, int Base = 0>
SquareArray<T, Len, Base> MakeSquareArray(const T& initialValue)
{
  SquareArray arr = MakeSquareArray<T, Len, Base>();
  arr.fill(initialValue);
  return arr;
}



template<typename T, int Len, int Base = 0>
class CubicArray
{
public:
  CubicArray()
    : m_Data(nullptr) {}
  ~CubicArray() { delete[] m_Data; }

  CubicArray(const CubicArray& other) = delete;
  CubicArray& operator=(const CubicArray& other) = delete;

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

  template<IntegerType IndexType>
  T& operator()(const IVec3<IndexType>& index)
  {
    return const_cast<T&>(static_cast<const CubicArray*>(this)->operator()(index));
  }

  template<IntegerType IndexType>
  const T& operator()(const IVec3<IndexType>& index) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    return m_Data[Len * Len * (index.i - Base) + Len * (index.j - Base) + (index.k - Base)];
  }

  const T* get() { return m_Data; }

  bool filledWith(const T& val) const
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    for (int i = 0; i < Size(); ++i)
      if (m_Data[i] != val)
        return false;
    return true;
  }

  template<IntegerType IndexType, int ArrLen, int ArrBase>
  bool contentsEqual(const IBox3<IndexType>& compareSection, const CubicArray<T, ArrLen, ArrBase>& arr, const IVec3<IndexType>& arrBase)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    if (!arr)
      return false;

    IVec3<IndexType> compareExtents = compareSection.extents();
    for (IndexType i = 0; i < compareExtents.i; ++i)
      for (IndexType j = 0; j < compareExtents.j; ++j)
        for (IndexType k = 0; k < compareExtents.k; ++k)
          if ((*this)[compareSection.min.i + i][compareSection.min.j + j][compareSection.min.k + k] != arr[arrBase.i + i][arrBase.j + j][arrBase.k + k])
            return false;
    return true;
  }

  void fill(const T& val)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");
    std::fill(m_Data, m_Data + Size(), val);
  }

  template<IntegerType IndexType>
  void fill(const IBox3<IndexType>& fillSection, const T& val)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    for (IndexType i = fillSection.min.i; i < fillSection.max.i; ++i)
      for (IndexType j = fillSection.min.j; j < fillSection.max.j; ++j)
        for (IndexType k = fillSection.min.k; k < fillSection.max.k; ++k)
          (*this)[i][j][k] = val;
  }

  template<IntegerType IndexType, int ArrLen, int ArrBase>
  void fill(const IBox3<IndexType>& fillSection, const CubicArray<T, ArrLen, ArrBase>& arr, const IVec3<IndexType>& arrBase)
  {
    EN_CORE_ASSERT(m_Data, "Data has not yet been allocated!");

    IVec3<IndexType> fillExtents = fillSection.extents();
    for (IndexType i = 0; i < fillExtents.i; ++i)
      for (IndexType j = 0; j < fillExtents.j; ++j)
        for (IndexType k = 0; k < fillExtents.k; ++k)
          (*this)[fillSection.min.i + i][fillSection.min.j + j][fillSection.min.k + k] = arr[arrBase.i + i][arrBase.j + j][arrBase.k + k];
  }

  void reset()
  {
    delete[] m_Data;
    m_Data = nullptr;
  }

  static constexpr int Length() { return Len; }
  static constexpr int Size() { return Len * Len * Len; }

private:
  T* m_Data;

  void allocate() { m_Data = new T[Size()]; }

  friend CubicArray MakeCubicArray<T, Len, Base>();
  friend CubicArray MakeCubicArray<T, Len, Base>(const T& initialValue);
};

template<typename T, int Len, int Base = 0>
CubicArray<T, Len, Base> MakeCubicArray()
{
  CubicArray<T, Len, Base> arr;
  arr.allocate();
  return arr;
}

template<typename T, int Len, int Base = 0>
CubicArray<T, Len, Base> MakeCubicArray(const T& initialValue)
{
  CubicArray arr = MakeCubicArray<T, Len, Base>();
  arr.fill(initialValue);
  return arr;
}