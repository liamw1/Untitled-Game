#pragma once
#include "Concepts.h"
#include "Policy.h"

namespace eng::algo
{
  namespace detail
  {
    template<std::forward_iterator I>
    using underlyingType = decltype(*std::declval<I>());

    template<Iterable C>
    using containedType = decltype(*std::begin(std::declval<C>()));
  }

  template<Iterable C, typename R, InvocableWithReturnType<R, detail::containedType<C>> F, BinaryOp<R, detail::containedType<C>> Op>
  R accumulate(const C& container, F&& transform, Op&& operation, R initialValue)
  {
    using ContainedType = detail::containedType<C>;
    return std::accumulate(std::begin(container), std::end(container), initialValue, [&transform, &operation](const R& a, const ContainedType& b) { return operation(a, transform(b)); });
  }

  template<Iterable C, typename R, TransformToAddable<R, detail::containedType<C>> F>
  R sum(const C& container, F&& transform, R initialValue)
  {
    using ContainedType = detail::containedType<C>;
    return std::accumulate(std::begin(container), std::end(container), initialValue, [&transform](const R& a, const ContainedType& b) { return a + transform(b); });
  }

  template<Iterable C, Predicate<detail::containedType<C>> P>
  auto findIf(C& container, P&& predicate) -> decltype(std::begin(container))
  {
    return std::find_if(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<std::forward_iterator I, TransformToComparable<detail::underlyingType<I>> F>
  void sort(I first, I last, F&& transform, SortPolicy sortPolicy)
  {
    using UnderlyingType = detail::underlyingType<I>;
    switch (sortPolicy)
    {
      case SortPolicy::Ascending:   std::sort(first, last, [&transform](const UnderlyingType& a, const UnderlyingType& b) { return transform(a) < transform(b); }); return;
      case SortPolicy::Descending:  std::sort(first, last, [&transform](const UnderlyingType& a, const UnderlyingType& b) { return transform(a) > transform(b); }); return;
    }
    throw std::invalid_argument("Invalid sort policy!");
  }

  template<Iterable C, TransformToComparable<detail::containedType<C>> F>
  void sort(C& container, F&& transform, SortPolicy sortPolicy)
  {
    sort(std::begin(container), std::end(container), std::forward<F>(transform), sortPolicy);
  }

  template<Iterable C, Predicate<detail::containedType<C>> P>
  auto partition(C& container, P&& predicate) -> decltype(std::begin(container))
  {
    return std::partition(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<Iterable C, std::convertible_to<detail::containedType<C>> T>
  void fill(C& container, T&& value)
  {
    std::fill(std::begin(container), std::end(container), std::forward<T>(value));
  }
}