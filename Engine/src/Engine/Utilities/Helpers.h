#pragma once
#include "Engine/Core/Concepts.h"

namespace eng
{
  template<typename T>
  using removeConstFromReference = std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>;

  template<typename T>
  using removeConst = std::conditional_t<std::is_reference_v<T>, removeConstFromReference<T>, std::remove_const_t<T>>;

  template<typename>
  [[maybe_unused]] constexpr bool AlwaysFalse = false;

  template<std::copyable T>
  constexpr T clone(const T& obj) { return obj; }

  template<MemberFunction F, Pointer P>
  auto bindMemberFunction(F memberFunction, P objectPointer)
  {
    return [memberFunction, objectPointer](auto&&... args)
    {
      return std::invoke(memberFunction, objectPointer, std::forward<decltype(args)>(args)...);
    };
  }
}