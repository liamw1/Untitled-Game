#pragma once
#include "Engine/Core/PlatformDetection.h"

// ======================== Debug Macros ======================== //
#ifdef EN_DEBUG
  #define EN_ENABLE_ASSERTS
#if defined(EN_PLATFORM_WINDOWS)
  #define EN_DEBUG_BREAK() __debugbreak()
#elif defined(EN_PLATFORM_LINUX)
  #include <signal.h>
    #define EN_DEBUG_BREAK() raise(SIGTRAP)
  #else
    #error "Platform doesn't support debugbreak yet!"
  #endif
#else
  #define EN_DEBUG_BREAK()
#endif

// Assert macros.  Will be removed from code in non-debug builds
#ifdef EN_ENABLE_ASSERTS
  #define EN_ASSERT(x, ...) { if (!(x)) { EN_FATAL(__VA_ARGS__); EN_DEBUG_BREAK(); } }
  #define EN_CORE_ASSERT(x, ...) { if (!(x)) { EN_CORE_FATAL(__VA_ARGS__); EN_DEBUG_BREAK(); } }
#else
  #define EN_ASSERT(x, ...)
  #define EN_CORE_ASSERT(x, ...)
#endif



/*
  Debug functions for use in assert macros.
*/
namespace Engine::Debug
{
  // Returns true if value is on the interval [a, b).
  template<std::three_way_comparable T, std::convertible_to<T> A, std::convertible_to<T> B>
  constexpr bool BoundsCheck(T value, A a, B b) { return a <= value && value < b; }

  // Returns true if value is equal to one of the arguments.
  template<std::equality_comparable T, std::convertible_to<T> Arg>
  constexpr bool EqualsOneOf(T value, Arg arg) { return value == arg; }
  template<std::equality_comparable T, std::convertible_to<T> F, std::convertible_to<T>... Args>
  constexpr bool EqualsOneOf(T value, F first, Args... args) { return value == first || EqualsOneOf(value, args...); }

  // Returns true if n doesn't overflow when multiplied by m.
  template<std::integral IntType, std::convertible_to<IntType> OtherIntType>
  constexpr bool OverflowCheck(IntType n, OtherIntType m) { return n == 0 || static_cast<IntType>(n * m) / n == m; }
}