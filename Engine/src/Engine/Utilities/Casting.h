#pragma once
#include "Engine/Core/Concepts.h"

namespace eng
{
  namespace detail
  {
    template<Arithmetic T>
    constexpr int rank()
    {
      if (std::is_same_v<T, bool>)
        return 0;
      else if constexpr (std::is_integral_v<T>)
        return sizeof(T);
      else
        return sizeof(iMax) + sizeof(T);
    }

    template<Arithmetic C, Arithmetic T>
    constexpr bool canSafelyCast(T value, C min = std::numeric_limits<C>::min(), C max = std::numeric_limits<C>::max())
    {
      if constexpr (std::is_signed_v<T> != std::is_signed_v<C>)
        return static_cast<T>(std::max(min, C())) <= value && value <= max;
      else
        return min <= value && value <= max;
    }
  }

  /*
    Used for 'safe' casts. That is, conversions from type A to type B, where values in A are always
    representable in B.
  */
  template<Arithmetic C, Arithmetic T>
  constexpr C upCast(T value)
  {
    static_assert(detail::rank<T>() < detail::rank<C>(), "Not an upcast!");
    return static_cast<C>(value);
  }

  template<Arithmetic C, Arithmetic T>
  constexpr C downCast(T value)
  {
    static_assert(detail::rank<T>() >= detail::rank<C>(), "Not a downcast!");
    if (detail::canSafelyCast<C>(value))
      return static_cast<C>(value);
    throw std::range_error("Value cannot be represented in the destination type!");
  }

  template<Arithmetic C, Arithmetic T>
  constexpr C downCastUnchecked(T value)
  {
    static_assert(detail::rank<T>() >= detail::rank<C>(), "Not a downcast!");
    ENG_CORE_ASSERT(detail::canSafelyCast<C>(value), "Value cannot be represented in the destination type!");
    return static_cast<C>(value);
  }

  template<IterableEnum E, Arithmetic T>
  constexpr E enumCast(T value)
  {
    if (detail::canSafelyCast(value, toUnderlying(E::First), toUnderlying(E::Last)))
      return static_cast<E>(value);
    throw std::range_error("Value cannot be represented by enum!");
  }

  template<IterableEnum E, Arithmetic T>
  constexpr E enumCastUnchecked(T value)
  {
    ENG_CORE_ASSERT(detail::canSafelyCast(value, toUnderlying(E::First), toUnderlying(E::Last)), "Value cannot be represented by enum!");
    return static_cast<E>(value);
  }
}