#pragma once
#include "Concepts.h"

namespace eng
{
  namespace detail
  {
    template<std::forward_iterator I>
    using underlyingType = std::remove_reference_t<decltype(*I())>;

    template<Iterable C>
    using containedType = std::remove_reference_t<decltype(*std::begin(C()))>;
  }

  template<std::forward_iterator I, typename F>
  void unarySort(I first, I last, const F& unary)
  {
    static_assert(std::is_invocable_r_v<size_t, F, detail::underlyingType<I>>, "'unary' is not a valid unary function on the underlying type!");
    std::sort(first, last, [&unary](const detail::underlyingType<I>& a, const detail::underlyingType<I>& b) { return unary(a) < unary(b); });
  }

  template<Iterable C, typename F>
  void unarySortContainer(C& container, const F& unary)
  {
    static_assert(std::is_invocable_r_v<size_t, F, detail::containedType<C>>, "'unary' is not a valid unary function on the contained type!");
    unarySort(std::begin(container), std::end(container), unary);
  }

  template<Iterable C, typename F>
  auto partitionContainer(C& container, const F& predicate) -> decltype(std::begin(container))
  {
    static_assert(std::is_invocable_r_v<bool, F, detail::containedType<C>>, "'predicate' is not a valid predicate on the contained type!");
    return std::partition(std::begin(container), std::end(container), predicate);
  }

  template<Iterable C, typename T>
  void fillContainer(C& container, const T& value)
  {
    static_assert(std::is_convertible_v<T, detail::containedType<C>>, "Given value cannot convert to contained type!");
    std::fill(std::begin(container), std::end(container), value);
  }
}