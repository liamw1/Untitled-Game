#pragma once
#include "Engine/Core/FixedWidthTypes.h"

namespace eng
{
  template<typename T>
  std::stack<T, std::vector<T>> vectorBasedStack(uSize initialCapacity)
  {
    std::vector<T> container;
    container.reserve(initialCapacity);
    return std::stack<T, std::vector<T>>(std::move(container));
  }
}