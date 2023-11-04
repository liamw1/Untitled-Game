#pragma once
#include "Concepts.h"
#include "Policy.h"

namespace eng::algo
{
  namespace detail
  {
    template<Iterable C>
    using containedType = std::iter_value_t<decltype(std::begin(std::declval<C>()))>;
  }

  template<Iterable C, typename R, InvocableWithReturnType<R, detail::containedType<C>> F, BinaryOp<R, detail::containedType<C>> Op>
  constexpr R accumulate(const C& container, F&& transform, Op&& operation, R initialValue)
  {
    using ValueType = detail::containedType<C>;
    return std::accumulate(std::begin(container), std::end(container), initialValue, [&transform, &operation](const R& a, const ValueType& b) { return operation(a, transform(b)); });
  }

  template<Iterable C, typename R, TransformToAddable<R, detail::containedType<C>> F>
  constexpr R sum(const C& container, F&& transform, R initialValue)
  {
    using ValueType = detail::containedType<C>;
    return std::accumulate(std::begin(container), std::end(container), initialValue, [&transform](const R& a, const ValueType& b) { return a + transform(b); });
  }

  template<Iterable C, std::predicate<detail::containedType<C>> P>
  constexpr bool allOf(const C& container, P&& predicate)
  {
    return std::all_of(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<Iterable C, std::predicate<detail::containedType<C>> P>
  constexpr bool anyOf(const C& container, P&& predicate)
  {
    return std::any_of(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<Iterable C, std::predicate<detail::containedType<C>> P>
  constexpr bool noneOf(const C& container, P&& predicate)
  {
    return std::none_of(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<Iterable C, std::predicate<detail::containedType<C>> P>
  constexpr auto findIf(C& container, P&& predicate) -> decltype(std::begin(container))
  {
    return std::find_if(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<std::random_access_iterator I, TransformToComparable<std::iter_value_t<I>> F>
  constexpr void sort(I first, I last, F&& transform, SortPolicy sortPolicy)
  {
    using ValueType = std::iter_value_t<I>;
    switch (sortPolicy)
    {
      case SortPolicy::Ascending:   std::sort(first, last, [&transform](const ValueType& a, const ValueType& b) { return transform(a) < transform(b); }); return;
      case SortPolicy::Descending:  std::sort(first, last, [&transform](const ValueType& a, const ValueType& b) { return transform(a) > transform(b); }); return;
    }
    throw std::invalid_argument("Invalid sort policy!");
  }

  template<IterableContainer C, TransformToComparable<detail::containedType<C>> F>
  constexpr void sort(C& container, F&& transform, SortPolicy sortPolicy)
  {
    sort(std::begin(container), std::end(container), std::forward<F>(transform), sortPolicy);
  }

  template<IterableContainer C, std::predicate<detail::containedType<C>> P>
  constexpr auto partition(C& container, P&& predicate) -> decltype(std::begin(container))
  {
    return std::partition(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<IterableContainer C, std::convertible_to<detail::containedType<C>> T>
  constexpr void fill(C& container, T&& value)
  {
    std::fill(std::begin(container), std::end(container), std::forward<T>(value));
  }

  template<IterableContainer C, Incrementable T>
  constexpr void iota(C& container, T value)
  {
    std::iota(std::begin(container), std::end(container), value);
  }

  template<IterableContainer C, IteratorWithValueType<detail::containedType<C>> D>
  constexpr void partialSum(C& container, D destination)
  {
    std::partial_sum(std::begin(container), std::end(container), destination);
  }
}