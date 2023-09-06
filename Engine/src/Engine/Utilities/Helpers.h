#pragma once

namespace Engine
{
  template<std::copyable T>
  T Clone(const T& obj) { return obj; }
}