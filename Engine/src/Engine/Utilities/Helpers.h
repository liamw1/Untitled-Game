#pragma once

/*
  A place for miscellaneous utilities that don't rely on any other engine headers.
*/
namespace eng
{
  template<typename T1, typename T2>
  using largestType = std::conditional_t<sizeof(T1) < sizeof(T2), T2, T1>;

  template<typename>
  [[maybe_unused]] constexpr bool AlwaysFalse = false;

  template<std::copyable T>
  constexpr T clone(const T& obj) { return obj; }

  template<std::copyable T>
  struct Identity
  {
    T operator()(const T& obj) const { return obj; } 
  };
}