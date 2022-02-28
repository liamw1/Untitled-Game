#pragma once
#include "Container.h"

/*
  A 2D-style array that stores its members in a single
  heap-allocated block in memory.
  Elements can be accessed with brackets:
  arr[i][j]
  Huge advantages in efficiency for "tall" arrays (rows >> cols).
*/
template<typename T>
class Array2D
{
public:
  Array2D()
    : Array2D(0, 0) {}

  Array2D(int size)
    : Array2D(size, size) {}

  Array2D(int size1, int size2)
    : container(Container<T>(size1, size2)) {}

  Array2D(const Array2D& other) = delete;

  Array2D(Array2D&& other) noexcept
    : container(std::move(other.container)) {}

  Array2D& operator=(const Array2D& other) = delete;

  Array2D& operator=(Array2D&& other) noexcept
  {
    if (&other != this)
      container = std::move(other.container);
    return *this;
  }

  Container<T>& operator[](int index)
  {
    container.index1 = index;
    return container;
  }

  const Container<T>& operator[](int index) const
  {
    container.index1 = index;
    return container;
  }

private:
  Container<T> container;
};


template<typename T>
void printArray(const Array2D<T>& data, int size1, int size2)
{
  for (int j = 0; j < size2; ++j)
  {
    std::cout << "|";
    for (int i = 0; i < size1; ++i)
    {
      std::cout << data[i][j] << " ";
    }
    std::cout << "|" << std::endl;
  }
  std::cout << std::endl;
}