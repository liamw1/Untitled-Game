#pragma once
#include "Engine/Core/Log/Log.h"

// ======================== Debug Macros ======================== //
#ifdef ENG_DEBUG
  #define ENG_ENABLE_ASSERTS
#if defined(ENG_PLATFORM_WINDOWS)
  #define ENG_DEBUG_BREAK() __debugbreak()
#elif defined(ENG_PLATFORM_LINUX)
  #include <signal.h>
    #define ENG_DEBUG_BREAK() raise(SIGTRAP)
  #else
    #error "Platform doesn't support debugbreak yet!"
  #endif
#else
  #define ENG_DEBUG_BREAK()
#endif

// Runtime assert macros.  Will be removed from code in non-debug builds
#ifdef ENG_ENABLE_ASSERTS
  #define ENG_ASSERT(x, ...) { if (!(x)) { ENG_FATAL(__VA_ARGS__); ENG_DEBUG_BREAK(); } }
  #define ENG_CORE_ASSERT(x, ...) { if (!(x)) { ENG_CORE_FATAL(__VA_ARGS__); ENG_DEBUG_BREAK(); } }
#else
  #define ENG_ASSERT(x, ...)
  #define ENG_CORE_ASSERT(x, ...)
#endif



/*
  Debug functions for use in assert macros.
*/
namespace eng::debug
{
  // Returns true if value is on the interval [a, b).
  template<std::three_way_comparable T, std::convertible_to<T> A, std::convertible_to<T> B>
  constexpr bool boundsCheck(T value, A a, B b) { return a <= value && value < b; }

  // Returns true if value is equal to one of the arguments.
  template<std::equality_comparable T, std::convertible_to<T>... Args>
  constexpr bool equalsOneOf(T value, Args... args) { return ((value == args) || ...); }

  // Returns true if n doesn't overflow when multiplied by m.
  // TODO: Remove
  template<std::integral IntType, std::convertible_to<IntType> OtherIntType>
  constexpr bool overflowCheck(IntType n, OtherIntType m) { return n == 0 || static_cast<IntType>(n * m) / n == m; }
}