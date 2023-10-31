#pragma once
#include "FixedWidthTypes.h"

namespace eng
{
  template<typename T, typename... Args>
  concept AnyOf = (std::same_as<T, Args> || ...);
  
  template<typename T, typename DecayedType>
  concept DecaysTo = std::same_as<std::decay_t<T>, DecayedType>;

  template<typename T>
  concept Arithmetic = std::is_arithmetic_v<T>;
  
  template<typename E>
  concept Enum = std::is_enum_v<E>;
  
  template<typename E>
  concept IterableEnum = Enum<E> && requires(E)
  {
    { E::First };
    { E::Last  };
  };

  // Return type must match exactly. If merely convertible to return type is desired, use std::is_invocable_r_v.
  template<typename F, typename ReturnType, typename... Args>
  concept InvocableWithReturnType = std::is_invocable_v<F, Args...> && std::same_as<ReturnType, std::invoke_result_t<F, Args...>>;

  template<typename F, typename... Args>
  concept Predicate = InvocableWithReturnType<F, bool, Args...>;
  
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

  template<typename T>
  concept Iterable = requires(T t)
  {
    { std::begin(t) } -> std::forward_iterator;
    { std::end(t)   } -> std::forward_iterator;
  };

  template<typename F, typename R, typename T>
  concept UnaryOp = InvocableWithReturnType<F, R, T>;

  template<typename F, typename R, typename T>
  concept BinaryOp = InvocableWithReturnType<F, R, T, T>;

  template<typename T>
  concept EqualityComparable = requires(T a, T b)
  {
    { a == b } -> std::same_as<bool>;
    { a != b } -> std::same_as<bool>;
  };

  template<typename T>
  concept LessThanComparable = requires(T a, T b)
  {
    { a < b } -> std::same_as<bool>;
  };

  template<typename F, typename T>
  concept TransformToComarable = std::invocable<F, T> && LessThanComparable<std::invoke_result_t<F, T>>;

  template<typename F, typename T>
  concept BinaryComparison = BinaryOp<F, bool, T>;

  template<typename T>
  concept Addable = requires(T a, T b)
  {
    { a + b } -> std::same_as<T>;
  };

  template<typename F, typename ReturnType, typename T>
  concept TransformToAddable = UnaryOp<F, ReturnType, T> && Addable<ReturnType>;
}