#pragma once
#include "Engine/Core/Concepts.h"

namespace Engine
{
  template<std::copyable T>
  constexpr T Clone(const T& obj) { return obj; }

  template<MemberFunction F, Pointer P>
  auto BindMemberFunction(F memberFunction, P objectPointer)
  {
    return [memberFunction, objectPointer](auto&&... args)
      {
        return std::invoke(memberFunction, objectPointer, std::forward<decltype(args)>(args)...);
      };
  }
}