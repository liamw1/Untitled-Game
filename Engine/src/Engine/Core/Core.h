#pragma once
#include "PlatformDetection.h"

/*
  *:･ﾟ✧*:･ﾟ✧ ~~ Macro/Template Magic Zone ~~ ✧･ﾟ: *✧･ﾟ:*
*/

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



// ================== Physical Units and Constants ================== //
using radians = float;
using radiansPerSec = radians;
using length_t = float;
using seconds = float;

constexpr radians PI = static_cast<radians>(3.14159265358979323846264338327950288419716939937510L);



// ======================== Common Utilities ======================== //
inline constexpr uint64_t bit(int n) { return 1Ui64 << n; }
#define EN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Engine
{
  template<typename T>
  using Unique = std::unique_ptr<T>;
  template<typename T, typename ... Args>
  constexpr Unique<T> CreateUnique(Args&& ... args)
  {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  template<typename T>
  using Shared = std::shared_ptr<T>;
  template<typename T, typename ... Args>
  constexpr Shared<T> CreateShared(Args&& ... args)
  {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }
}



// ==================== Enabling Bitmasking for Enum Classes ==================== //
// Code borrowed from: https://wiggling-bits.net/using-enum-classes-as-type-safe-bitmasks/
#define EN_ENABLE_BITMASK_OPERATORS(x)\
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



// ==================== Enabling Iteration for Enum Classes ==================== //
// Code borrowed from: https://stackoverflow.com/questions/261963/how-can-i-iterate-over-an-enum
template<typename Enum, Enum beginEnum, Enum endEnum>
class Iterator
{
  using val_t = typename std::underlying_type<Enum>::type;

public:
  Iterator(Enum valEnum)
    : value(static_cast<val_t>(valEnum)) {}
  Iterator()
    : value(static_cast<val_t>(beginEnum)) {}

  Iterator& operator++()
  {
    ++value;
    return *this;
  }
  Enum operator*() { return static_cast<Enum>(value); }
  bool operator!=(const Iterator& other) { return value != other.value; }

  Iterator begin() { return *this; }
  Iterator end()
  {
    static const Iterator endIter = ++Iterator(endEnum);
    return endIter;
  }

private:
  val_t value;
};