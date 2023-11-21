#pragma once
#include "Engine/Utilities/Helpers.h"
#include "Engine/Core/Concepts.h"

namespace eng::math
{
  template<Arithmetic T>
  [[nodiscard]] constexpr T square(T value) noexcept { return value * value; }

  template<Arithmetic T>
  [[nodiscard]] constexpr T cube(T value) noexcept { return value * square(value); }

  template<std::integral T>
  [[nodiscard]] constexpr T pow2(T n) noexcept { return static_cast<T>(1) << n; }

  template<Arithmetic T>
  [[nodiscard]] constexpr T abs(T value) noexcept
  {
    if constexpr (std::is_signed_v<T>)
      return std::abs(value);
    else
      return value;
  }

  // Euclidean modulus
  template<std::integral T1, std::integral T2>
  [[nodiscard]] constexpr auto mod(T1 left, T2 right) noexcept -> decltype(left % right)
  {
    ENG_CORE_ASSERT(right != 0, "Right operand of euclidean modulus cannot be 0!");
    auto result = left % right;
    if constexpr (std::is_signed_v<decltype(result)>)
      return result >= 0 ? result : result + abs(right);
    else
      return result;
  }
}