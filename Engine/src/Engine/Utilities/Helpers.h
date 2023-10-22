#pragma once

namespace eng
{
  template<typename T>
  using removeConstFromReference = std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>;

  template<typename T>
  using removeConst = std::conditional_t<std::is_reference_v<T>, removeConstFromReference<T>, std::remove_const_t<T>>;

  template<typename T1, typename T2>
  using largest = std::conditional_t<sizeof(T1) < sizeof(T2), T2, T1>;

  template<typename>
  [[maybe_unused]] constexpr bool AlwaysFalse = false;

  template<std::copyable T>
  constexpr T clone(const T& obj) { return obj; }
}