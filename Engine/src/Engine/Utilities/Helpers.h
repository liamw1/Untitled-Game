#pragma once
#include "Engine/Core/Concepts.h"

namespace eng
{
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