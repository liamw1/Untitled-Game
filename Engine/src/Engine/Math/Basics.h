#pragma once
#include "Engine/Core/Casting.h"

namespace eng::math
{
  template<Arithmetic T>
  [[nodiscard]] constexpr T square(T value) noexcept { return value * value; }

  template<Arithmetic T>
  [[nodiscard]] constexpr T cube(T value) noexcept { return value * square(value); }

  template<std::integral T>
  [[nodiscard]] constexpr T pow2(T value) noexcept { return static_cast<T>(1) << value; }

  template<Arithmetic T>
  [[nodiscard]] constexpr T abs(T value) noexcept
  {
    if constexpr (std::is_signed_v<T>)
      return std::abs(value);
    else
      return value;
  }

  template<typename T>
  [[nodiscard]] constexpr T lerp(const T& a, const T& b, f32 t) { return (1 - t) * a + t * b; }

  /*
    Euclidean modulus. Return type may get upgraded if divisor cannot be represented
    in the type of the dividend.
  */
  template<uSize N, std::integral T>
  [[nodiscard]] constexpr auto mod(T value) noexcept
  {
    static_assert(N != 0, "Divisor cannot be 0!");

    if constexpr (std::is_signed_v<T> && !exactlyRepresentable<T, N>())
    {
      static_assert(exactlyRepresentable<iMax, N>(), "Divisor cannot be represented in largest signed integer type!");
      iMax result = value % static_cast<iMax>(N);
      return static_cast<uSize>(result >= 0 ? result : result + static_cast<iMax>(N));
    }
    else if constexpr (std::is_signed_v<T>)
    {
      T result = value % static_cast<T>(N);
      return result >= 0 ? result : result + static_cast<T>(N);
    }
    else
      return value % N;
  }
}