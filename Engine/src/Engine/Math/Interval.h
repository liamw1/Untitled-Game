#pragma once

#include <concepts>
#include <ostream>

namespace eng::math
{
  template<std::floating_point T>
  struct Interval
  {
    T min;
    T max;
  
    constexpr Interval()
      : min(0), max(0) {}
    constexpr Interval(T minValue, T maxValue)
      : min(minValue), max(maxValue) {}

    constexpr T length() const { return max - min; }
  };
}

namespace std
{
  template<std::floating_point T>
  inline ostream& operator<<(ostream& os, const eng::math::Interval<T>& interval)
  {
    return os << '[' << interval.min << ", " << interval.max << ']';
  }
}