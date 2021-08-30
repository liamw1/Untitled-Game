#pragma once

// ==================== Common Utilities ==================== //
#define EN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// ==================== Debug Macros ==================== //
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

// ==================== Windows Specific Macros ==================== //
#ifdef EN_PLATFORM_WINDOWS
  #if EN_DYNAMIC_LINK
    #ifdef EN_BUILD_DLL
      #define ENGINE_API __declspec(dllexport)
    #else
      #define ENGINE_API __declspec(dllimport)
    #endif
  #else
    #define ENGINE_API
  #endif
#else
  #error Engine only supports Windows!
#endif

// ==================== For Disabling Warnings ==================== //
#ifdef _MSC_VER
  // 4251 - 'class 'type1' needs to have dll-interface to be used by clients of class 'type2'
  #pragma warning( disable : 4251)
#endif