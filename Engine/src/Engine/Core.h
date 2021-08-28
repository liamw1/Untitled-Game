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

/*
  For disabling annoyting warnings.  Do this very sparingly.
*/
#ifdef _MSC_VER
  // 4251 - 'class 'type1' needs to have dll-interface to be used by clients of class 'type2'
  #pragma warning( disable : 4251)
#endif