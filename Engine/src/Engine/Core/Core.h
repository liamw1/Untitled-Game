#pragma once

// ======================== Platform Detection ======================== //
#ifdef _WIN32
  /* Windows x64/x86 */
  #ifdef _WIN64
    /* Windows x64  */
    #define EN_PLATFORM_WINDOWS
  #else
    /* Windows x86 */
    #error "x86 Builds are not supported!"
  #endif
#elif defined(__APPLE__) || defined(__MACH__)
  #include <TargetConditionals.h>
  /* TARGET_OS_MAC exists on all the platforms
   * so we must check all of them (in this order)
   * to ensure that we're running on MAC
   * and not some other Apple platform */
  #if TARGET_IPHONE_SIMULATOR == 1
    #error "IOS simulator is not supported!"
  #elif TARGET_OS_IPHONE == 1
    #define EN_PLATFORM_IOS
    #error "IOS is not supported!"
  #elif TARGET_OS_MAC == 1
    #define EN_PLATFORM_MACOS
    #error "MacOS is not supported!"
  #else
    #error "Unknown Apple platform!"
  #endif
 /* We also have to check __ANDROID__ before __linux__
  * since android is based on the linux kernel
  * it has __linux__ defined */
#elif defined(__ANDROID__)
  #define EN_PLATFORM_ANDROID
  #error "Android is not supported!"
#elif defined(__linux__)
  #define EN_PLATFORM_LINUX
  #error "Linux is not supported!"
#else
  /* Unknown compiler/platform */
  #error "Unknown platform!"
#endif



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



// ==================== Enabling Bitmasking for Enum Classes ==================== //
// Code borrowed from: https://wiggling-bits.net/using-enum-classes-as-type-safe-bitmasks/
#define ENABLE_BITMASK_OPERATORS(x)   \
template<>                            \
struct EnableBitMaskOperators<x>      \
{                                     \
  static const bool enable = true;    \
};                                    \

template<typename Enum>
struct EnableBitMaskOperators
{
  static const bool enable = false;
};

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator&(Enum enumA, Enum enumB)
{
  return static_cast<Enum>(static_cast<std::underlying_type<Enum>::type>(enumA) &
                           static_cast<std::underlying_type<Enum>::type>(enumB));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
operator|(Enum enumA, Enum enumB)
{
  return static_cast<Enum>(static_cast<std::underlying_type<Enum>::type>(enumA) |
                           static_cast<std::underlying_type<Enum>::type>(enumB));
}



// ==================== Common Utilities ==================== //
inline constexpr uint8_t bit(uint8_t n) { return 1 << n; }
#define EN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Engine
{
  template<typename T>
  using Unique = std::unique_ptr<T>;
  template<typename T, typename ... Args>
  constexpr Unique<T> createUnique(Args&& ... args)
  {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  template<typename T>
  using Shared = std::shared_ptr<T>;
  template<typename T, typename ... Args>
  constexpr Shared<T> createShared(Args&& ... args)
  {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }
}