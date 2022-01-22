#pragma once

/*
  A helper class that stores the data for Array2D.
*/
template<typename T>
struct Container
{
public:
  mutable int index1;

  Container() = delete;

  Container(const int size1, const int size2)
    : M(size2), index1(0)
  {
    data = new T[size1 * static_cast<int64_t>(size2)];
  }

  Container(const Container& other) = delete;

  Container(Container&& other) noexcept
    : M(other.M), index1(other.index1)
  {
    data = other.data;
    other.data = nullptr;
  }

  Container& operator=(const Container& other) = delete;

  Container& operator=(Container&& other) noexcept
  {
    if (&other != this)
    {
      M = other.M;
      index1 = other.index1;

      delete[] data;
      data = other.data;
      other.data = nullptr;
    }
    return *this;
  }

  T& operator[](const int index2)
  {
    return data[index1 * M + index2];
  }

  const T& operator[](const int index2) const
  {
    return data[index1 * M + index2];
  }

  ~Container()
  {
    delete[] data;
  }

private:
  int M;
  T* data = nullptr;
};