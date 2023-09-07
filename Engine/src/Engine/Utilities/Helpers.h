#pragma once

namespace Engine
{
  template<std::copyable T>
  constexpr T Clone(const T& obj) { return obj; }
}