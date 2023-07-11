#pragma once
#include "PlatformDetection.h"

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



// Returns true if val is on the interval [a, b)
constexpr bool boundsCheck(int val, int a, int b) { return a <= val && val < b; }

#define EN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }