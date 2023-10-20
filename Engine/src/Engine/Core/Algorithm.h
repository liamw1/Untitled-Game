#pragma once
#include "Concepts.h"
#include "Policy.h"

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
    requires std::invocable<F, detail::underlyingType<I>> &&
             LessThanComparable<std::invoke_result_t<F, detail::underlyingType<I>>>
  void sort(I first, I last, const F& unary, SortPolicy sortPolicy)
  {
    using UnderlyingType = detail::underlyingType<I>;
    switch (sortPolicy)
    {
      case SortPolicy::Ascending:   std::sort(first, last, [&unary](const UnderlyingType& a, const UnderlyingType& b) { return   unary(a) < unary(b);  }); break;
      case SortPolicy::Descending:  std::sort(first, last, [&unary](const UnderlyingType& a, const UnderlyingType& b) { return !(unary(a) < unary(b)); }); break;
      default:                      throw std::invalid_argument("Unknown sort policy!");
    }
  }

  template<Iterable C, typename F>
    requires std::invocable<F, detail::containedType<C>> &&
             LessThanComparable<std::invoke_result_t<F, detail::containedType<C>>>
  void sortContainer(C& container, const F& unary, SortPolicy sortPolicy)
  {
    sort(std::begin(container), std::end(container), unary, sortPolicy);
  }

  template<Iterable C, typename F>
    requires InvocableWithReturnType<F, bool, detail::containedType<C>>
  auto partitionContainer(C& container, const F& predicate) -> decltype(std::begin(container))
  {
    return std::partition(std::begin(container), std::end(container), predicate);
  }

  template<Iterable C, typename T>
    requires std::convertible_to<T, detail::containedType<C>>
  void fillContainer(C& container, const T& value)
  {
    std::fill(std::begin(container), std::end(container), value);
  }
}