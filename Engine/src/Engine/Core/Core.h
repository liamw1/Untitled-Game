#pragma once

#ifdef EN_PLATFORM_WINDOWS
  #ifdef EN_BUILD_DLL
    #define ENGINE_API __declspec(dllexport)
  #else
    #define ENGINE_API __declspec(dllimport)
  #endif
#else
  #error Engine only supports Windows!
#endif

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

/*
  For disabling annoyting warnings.  Do this very sparingly.
*/
#ifdef _MSC_VER
  // 4251 - 'class 'type1' needs to have dll-interface to be used by clients of class 'type2'
  #pragma warning( disable : 4251)
#endif