#pragma once
#include "FixedWidthTypes.h"

namespace eng
{
  template<typename T, typename... Args>
  concept AnyOf = (std::same_as<T, Args> || ...);

  template<typename T, typename... Args>
  concept NoneOf = !AnyOf<T, Args...>;
  
  template<typename T, typename DecayedType>
  concept DecaysTo = std::same_as<std::decay_t<T>, DecayedType>;

  template<typename T, typename AssignedType>
  concept AssignableTo = requires(AssignedType lhs, T rhs)
  {
    { lhs = std::forward<T>(rhs) } -> DecaysTo<AssignedType>;
  };

  template<typename T>
  concept Arithmetic = std::is_arithmetic_v<T>;
  
  template<typename E>
  concept Enum = std::is_enum_v<E>;
  
  template<typename E>
  concept IterableEnum = Enum<E> && requires(E)
  {
    { E::First };
    { E::Last  };
  } && static_cast<std::underlying_type_t<E>>(E::Last) > static_cast<std::underlying_type_t<E>>(E::First);

  template<typename T>
  concept StandardLayout = std::is_standard_layout_v<T>;

  // Return type must match exactly. If merely convertible to return type is desired, use std::is_invocable_r_v.
  template<typename F, typename ReturnType, typename... Args>
  concept InvocableWithReturnType = std::is_invocable_v<F, Args...> && std::same_as<ReturnType, std::invoke_result_t<F, Args...>>;
  
  template<typename F>
  concept MemberFunction = std::is_member_function_pointer_v<F>;
  
  template<typename P>
  concept Pointer = std::is_pointer_v<P>;

  template<typename T>
  concept Hashable = requires(T instance)
  {
    { std::hash<T>{}(instance) } -> std::convertible_to<uSize>;
  };
  
  template <typename T, typename ReturnType, typename IndexType>
  concept Indexable = requires(T t) { { t(IndexType()) } -> DecaysTo<ReturnType>; };

  template<typename I, typename T>
  concept IteratorWithValueType = std::same_as<std::iter_value_t<I>, T>;

  template<typename T>
  concept Iterable = requires(T t)
  {
    { std::begin(t) } -> std::forward_iterator;
    { std::end(t)   } -> std::forward_iterator;
  };

  template<typename T>
  concept IterableContainer = Iterable<T> && requires(T t)
  {
    { std::begin(t) } -> std::indirectly_writable<std::remove_cvref_t<decltype(*std::begin(t))>>;
    { std::end(t)   } -> std::indirectly_writable<std::remove_cvref_t<decltype(*std::end(t))>>;
  };

  template<typename T>
  concept Incrementable = requires(T t)
  {
    { ++t } -> std::same_as<T>;
    { t++ } -> std::same_as<T>;
  };

  template<typename F, typename T>
  concept UnaryOperationOn = InvocableWithReturnType<F, T, T>;

  template<typename F, typename T>
  concept BinaryOperationOn = InvocableWithReturnType<F, T, T, T>;

  template<typename F, typename T>
  concept BinaryRelation = std::relation<F, T, T>;

  template<typename F, typename T>
  concept TransformToComparable = std::invocable<F, T> && std::three_way_comparable<std::invoke_result_t<F, T>>;

  template<typename T>
  concept Addable = requires(T a, T b)
  {
    { a + b } -> std::same_as<T>;
  };

  template<typename F, typename T>
  concept TransformToAddable = std::invocable<F, T> && Addable<std::invoke_result_t<F, T>>;
}