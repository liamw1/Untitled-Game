#pragma once
#include "Concepts.h"
#include "Exception.h"
#include "Engine/Debug/Assert.h"

namespace eng
{
  namespace detail
  {
    static constexpr std::string_view c_BadCastMessage = "Value cannot be represented in the destination type!";

    template<Arithmetic C, Arithmetic T>
    constexpr bool canSafelyCast(T value, C min = std::numeric_limits<C>::lowest(), C max = std::numeric_limits<C>::max())
    {
      if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<C>)
        return min <= value && value <= max;
      else
        return std::cmp_less_equal(min, value) && std::cmp_less_equal(value, max);
    }

    // C++23: This can be replaced with std::to_underlying
    template<Enum E>
    constexpr std::underlying_type_t<E> toUnderlying(E e) { return static_cast<std::underlying_type_t<E>>(e); }
  }

  /*
    Checks whether values of type T are always on the interval spanning the range of values
    of type C. I call this propery 'weakly representable'. This is opposed to strongly
    representable, where values of type T are always exactly representable in C.
  */
  template<Arithmetic C, Arithmetic T>
  constexpr bool weaklyRepresentable()
  {
    return static_cast<fMax>(std::numeric_limits<C>::lowest()) <= static_cast<fMax>(std::numeric_limits<T>::lowest()) &&
           static_cast<fMax>(std::numeric_limits<C>::max()) >= static_cast<fMax>(std::numeric_limits<T>::max());
  }

  /*
    Checks whether the value N is exaclty representable in T.
  */
  template<std::integral T, uSize N>
  constexpr bool exactlyRepresentable()
  {
    return std::cmp_less_equal(std::numeric_limits<T>::lowest(), N) && std::cmp_less_equal(N, std::numeric_limits<T>::max());
  }

  /*
    Used casts that are always safe. That is, conversions from type A to type B, where values in
    A are weakly representable in B.
  */
  template<Arithmetic C, Arithmetic T>
  constexpr C arithmeticUpcast(T value)
  {
    static_assert(weaklyRepresentable<C, T>(), "Not an upcast!");
    return static_cast<C>(value);
  }

  template<Arithmetic C, Arithmetic T>
  constexpr C arithmeticCast(T value)
  {
    if constexpr (weaklyRepresentable<C, T>())
      return arithmeticUpcast<C>(value);
    else if (detail::canSafelyCast<C>(value))
      return static_cast<C>(value);
    else
      throw CoreException(detail::c_BadCastMessage.data());
  }

  template<Arithmetic C, Arithmetic T>
  constexpr C arithmeticCastUnchecked(T value)
  {
    if constexpr (weaklyRepresentable<C, T>())
      return arithmeticUpcast<C>(value);
    else
    {
      ENG_CORE_ASSERT(detail::canSafelyCast<C>(value), detail::c_BadCastMessage);
      return static_cast<C>(value);
    }
  }

  template<IterableEnum E, Arithmetic T>
  constexpr E enumCast(T value)
  {
    if (detail::canSafelyCast(value, detail::toUnderlying(E::First), detail::toUnderlying(E::Last)))
      return static_cast<E>(value);
    throw CoreException("Value cannot be represented by enum!");
  }

  template<IterableEnum E, Arithmetic T>
  constexpr E enumCastUnchecked(T value)
  {
    ENG_CORE_ASSERT(detail::canSafelyCast(value, detail::toUnderlying(E::First), detail::toUnderlying(E::Last)), detail::c_BadCastMessage);
    return static_cast<E>(value);
  }
}