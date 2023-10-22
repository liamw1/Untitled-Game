#pragma once
#include "Engine/Core/Concepts.h"

namespace eng
{
  template<MemberFunction F, Pointer P>
  auto bindMemberFunction(F memberFunction, P objectPointer)
  {
    return [memberFunction, objectPointer](auto&&... args)
    {
      return std::invoke(memberFunction, objectPointer, std::forward<decltype(args)>(args)...);
    };
  }
}