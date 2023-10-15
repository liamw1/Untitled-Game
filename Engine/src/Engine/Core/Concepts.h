#pragma once

namespace eng
{
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
  
  template<typename T, typename DecayedType>
  concept DecaysTo = std::same_as<std::decay_t<T>, DecayedType>;
  
  template<typename E>
  concept Enum = std::is_enum_v<E>;
  
  template<typename E>
  concept IterableEnum = Enum<E> && requires(E)
  {
    { E::Begin } -> std::same_as<E&>;
    { E::End   } -> std::same_as<E&>;
  };
  
  template<typename T>
  concept Hashable = requires(T instance)
  {
    { std::hash<T>{}(instance) } -> std::convertible_to<std::size_t>;
  };
  
  template <typename T, typename ReturnType, typename IndexType>
  concept Indexable = requires(T t) { { t(IndexType()) } -> DecaysTo<ReturnType>; };
  
  template<typename F, typename ReturnType, typename... Args>
  concept InvocableWithReturnType = std::is_invocable_r_v<ReturnType, F, Args...>;
  
  template<typename F>
  concept MemberFunction = std::is_member_function_pointer_v<F>;
  
  template<typename P>
  concept Pointer = std::is_pointer_v<P>;
}