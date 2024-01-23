#pragma once
#include "Concepts.h"
#include "Exception.h"
#include "Policy.h"

/*
  Container-based versions of std algorithms.
*/
namespace eng::algo
{
  namespace detail
  {
    template<Iterable C>
    using containedType = std::iter_value_t<decltype(std::begin(std::declval<C>()))>;

    template<Iterable C, std::invocable<detail::containedType<C>> F>
    using transformResult = std::invoke_result_t<F, detail::containedType<C>>;
  }

  template<Iterable C>
  constexpr std::vector<detail::containedType<C>> asVector(const C& container)
  {
    return std::vector<detail::containedType<C>>(std::begin(container), std::end(container));
  }

  /*
    \returns The accumulated transformed elements of the given container with an initial value, i.e. the result of
    initialValue operation transform(a_1) operation transform(a_2) operation ... operation transform(a_n).
  */
  template<Iterable C, std::invocable<detail::containedType<C>> F, BinaryOperationOn<detail::transformResult<C, F>> Op>
  constexpr detail::transformResult<C, F> accumulate(const C& container, F&& transform, Op&& operation, detail::transformResult<C, F> initialValue)
  {
    return std::accumulate(std::begin(container), std::end(container), initialValue, [&transform, &operation](const detail::transformResult<C, F>& a, const detail::containedType<C>& b)
    {
      return operation(a, transform(b));
    });
  }

  /*
    \returns The accumulated transformed elements of the given container, i.e. the result of
    transform(a_1) operation transform(a_2) operation ... operation transform(a_n).
  */
  template<Iterable C, std::invocable<detail::containedType<C>> F, BinaryOperationOn<detail::transformResult<C, F>> Op>
  constexpr detail::transformResult<C, F> accumulate(const C& container, F&& transform, Op&& operation)
  {
    using R = detail::transformResult<C, F>;
    return accumulate(container, std::forward<F>(transform), std::forward<Op>(operation), R());
  }

  /*
    \returns The sum of transformed elements of the given container with an initial value, i.e. the result of
    initialValue + transform(a_1) + transform(a_2) + ... + transform(a_n).
  */
  template<Iterable C, TransformToAddable<detail::containedType<C>> F>
  constexpr detail::transformResult<C, F> accumulate(const C& container, F&& transform, detail::transformResult<C, F> initialValue)
  {
    using R = detail::transformResult<C, F>;
    return accumulate(container, std::forward<F>(transform), std::plus<R>(), initialValue);
  }

  /*
    \returns The sum of transformed elements of the given container, i.e. the result of
    transform(a_1) + transform(a_2) + ... + transform(a_n).
  */
  template<Iterable C, TransformToAddable<detail::containedType<C>> F>
  constexpr detail::transformResult<C, F> accumulate(const C& container, F&& transform)
  {
    using R = detail::transformResult<C, F>;
    return accumulate(container, std::forward<F>(transform), std::plus<R>(), R());
  }

  /*
    \returns A generalized accumulation of transformed elements of the given container with an initial vaue.

    WARNING: Behavior is non-deterministic if given operation is not associative or not commutative,
             which includes subtraction, division, and most floating-point arithmetic operations.
  */
  template<Iterable C, std::invocable<detail::containedType<C>> F, BinaryOperationOn<detail::transformResult<C, F>> Op>
  constexpr detail::transformResult<C, F> reduce(const C& container, F&& transform, Op&& operation, detail::transformResult<C, F> initialValue)
  {
    return std::reduce(std::begin(container), std::end(container), initialValue, [&transform, &operation](const detail::transformResult<C, F>& a, const detail::containedType<C>& b)
    {
      return operation(a, transform(b));
    });
  }

  /*
    \returns A generalized accumulation of transformed elements of the given container.

    WARNING: Behavior is non-deterministic if given operation is not associative or not commutative,
             which includes subtraction, division, and most floating-point arithmetic operations.
  */
  template<Iterable C, std::invocable<detail::containedType<C>> F, BinaryOperationOn<detail::transformResult<C, F>> Op>
  constexpr detail::transformResult<C, F> reduce(const C& container, F&& transform, Op&& operation)
  {
    using R = detail::transformResult<C, F>;
    return reduce(container, std::forward<F>(transform), std::forward<Op>(operation), R());
  }

  /*
    \returns A generalized sum of transformed elements of the given container with an initial value.

    WARNING: Behavior is non-deterministic if addition on the transformed type is not associative or not commutative,
             which includes floating-point addition.
  */
  template<Iterable C, TransformToAddable<detail::containedType<C>> F>
  constexpr detail::transformResult<C, F> reduce(const C& container, F&& transform, detail::transformResult<C, F> initialValue)
  {
    using R = detail::transformResult<C, F>;
    return reduce(container, std::forward<F>(transform), std::plus<R>(), initialValue);
  }

  /*
    \returns A generalized sum of transformed elements of the given container.

    WARNING: Behavior is non-deterministic if addition on the transformed type is not associative or not commutative,
             which includes floating-point addition.
  */
  template<Iterable C, TransformToAddable<detail::containedType<C>> F>
  constexpr detail::transformResult<C, F> reduce(const C& container, F&& transform)
  {
    using R = detail::transformResult<C, F>;
    return reduce(container, std::forward<F>(transform), std::plus<R>(), R());
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

  /*
    \returns An iterator to the first element where transform(element) < value.
             WARNING: Behaviour is undefined if the container is not sorted.
  */
  template<Iterable C, TransformToComparable<detail::containedType<C>> F>
  constexpr auto lowerBound(const C& container, const std::invoke_result_t<F, detail::containedType<C>>& value, F&& transform) -> decltype(std::begin(container))
  {
    return std::lower_bound(std::begin(container), std::end(container), value,
                            [&transform](const detail::containedType<C>& a, const std::invoke_result_t<F, detail::containedType<C>>& b) { return transform(a) < b; });
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
    throw CoreException("Invalid sort policy!");
  }

  template<IterableContainer C, TransformToComparable<detail::containedType<C>> F>
  constexpr void sort(C& container, F&& transform, SortPolicy sortPolicy)
  {
    sort(std::begin(container), std::end(container), std::forward<F>(transform), sortPolicy);
  }

  template<IterableContainer C>
  constexpr void reverse(C& container)
  {
    std::reverse(std::begin(container), std::end(container));
  }

  template<IterableContainer C, std::predicate<detail::containedType<C>> P>
  constexpr auto partition(C& container, P&& predicate) -> decltype(std::begin(container))
  {
    return std::partition(std::begin(container), std::end(container), std::forward<P>(predicate));
  }

  template<IterableContainer C, std::predicate<detail::containedType<C>> P>
  constexpr auto removeIf(C& container, P&& predicate) -> decltype(std::begin(container))
  {
    return std::remove_if(std::begin(container), std::end(container), std::forward<P>(predicate));
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

  template<Iterable C, IteratorWithValueType<detail::containedType<C>> D>
  constexpr void copy(const C& container, D destination)
  {
    std::copy(std::begin(container), std::end(container), destination);
  }

  template<Iterable C, IteratorWithValueType<detail::containedType<C>> D>
  constexpr void partialSum(const C& container, D destination)
  {
    std::partial_sum(std::begin(container), std::end(container), destination);
  }
}