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
using length_t = float;
using seconds = float;

// ======= Precision selection for floating point tolerance ========= //
static constexpr length_t LNGTH_EPSILON = std::is_same<double, length_t>::value ? DBL_EPSILON : FLT_EPSILON;

constexpr length_t PI = static_cast<length_t>(3.14159265358979323846264338327950288419716939937510L);



// ======================== Common Utilities ======================== //
constexpr uint8_t bit8(int n) { return 1Ui8 << n; }
constexpr uint16_t bit16(int n) { return 1Ui16 << n; }
constexpr uint32_t bit32(int n) { return 1Ui32 << n; }
constexpr uint64_t bit64(int n) { return 1Ui64 << n; }

constexpr uint64_t bit(int n) { return bit64(n); }
constexpr uint64_t pow2(int n) { return bit64(n); }
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

  class Timestep
  {
  public:
    Timestep()
      : m_Timestep(std::chrono::duration<seconds>::zero()) {}

    Timestep(std::chrono::duration<seconds> duration)
      : m_Timestep(duration) {}

    seconds sec() const { return m_Timestep.count(); }

  private:
    std::chrono::duration<seconds> m_Timestep;
  };

  class Angle
  {
  public:
    Angle() = default;

    constexpr explicit Angle(float degrees)
      : m_Degrees(degrees) {}

    constexpr float deg() const { return m_Degrees; }
    constexpr float rad() const { return 0.01745329238f * m_Degrees; }

    operator float& () = delete;

    constexpr bool operator>(const Angle& rhs) const { return m_Degrees > rhs.m_Degrees; }
    constexpr bool operator<(const Angle& rhs) const { return m_Degrees < rhs.m_Degrees; }
    constexpr bool operator>=(const Angle& rhs) const { return m_Degrees >= rhs.m_Degrees; }
    constexpr bool operator<=(const Angle& rhs) const { return m_Degrees <= rhs.m_Degrees; }
    constexpr bool operator==(const Angle& rhs) const { return m_Degrees == rhs.m_Degrees; }

    constexpr Angle operator-() const { return Angle(-m_Degrees); }

    constexpr Angle operator*(int n) const { return Angle(n * m_Degrees); }
    constexpr Angle operator*(float x) const { return Angle(x * m_Degrees); }

    constexpr Angle& operator+=(const Angle& rhs)
    {
      m_Degrees += rhs.m_Degrees;
      return *this;
    }

    constexpr Angle& operator-=(const Angle& rhs)
    {
      m_Degrees -= rhs.m_Degrees;
      return *this;
    }

    static constexpr Angle PI() { return Angle(180.0f); }
    static constexpr Angle FromRad(float rad) { return Angle(57.2957795f * rad); }

  private:
    float m_Degrees;
  };
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