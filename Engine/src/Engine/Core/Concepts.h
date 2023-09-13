#pragma once

namespace detail
{
  template<int... args>
  constexpr bool allPositive()
  {
    for (int n : { args... })
      if (n <= 0)
        return false;
    return true;
  }
}

template<int N>
concept positive = N > 0;

template<int... args>
concept allPositive = detail::allPositive<args...>();

template<typename T>
concept Hashable = requires(T instance)
{
  { std::hash<T>{}(instance) } -> std::convertible_to<std::size_t>;
};

template<typename T, typename DecayedType>
concept DecaysTo = std::same_as<std::decay_t<T>, DecayedType>;

template<typename P>
concept Pointer = std::is_pointer_v<P>;

template<typename F>
concept MemberFunction = std::is_member_function_pointer_v<F>;

template<typename F, typename ReturnType, typename... Args>
concept InvocableWithReturnType = std::is_invocable_r_v<ReturnType, F, Args...>;

template <typename T, typename ReturnType, typename IndexType>
concept Indexable = requires(T t) { { t(IndexType()) } -> DecaysTo<ReturnType>; };