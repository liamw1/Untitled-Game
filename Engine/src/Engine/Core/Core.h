#pragma once
#include "PlatformDetection.h"
#include <chrono>

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
using rad_t = float;
using seconds = float;

constexpr length_t operator"" _m(long double x) { return static_cast<length_t>(x); }
constexpr length_t operator"" _m(uint64_t x) { return static_cast<length_t>(x); }

namespace Constants
{
  constexpr length_t PI = static_cast<length_t>(3.1415926535897932384626433832795028841971693993751L);
  constexpr length_t SQRT2 = static_cast<length_t>(1.414213562373095048801688724209698078569671875377L);
  constexpr length_t SQRT3 = static_cast<length_t>(1.7320508075688772935274463415058723669428052538104L);
}



// ======================== Common Utilities ======================== //
constexpr uint8_t bitUi8(int n) { return 1Ui8 << n; }
constexpr uint16_t bitUi16(int n) { return 1Ui16 << n; }
constexpr uint32_t bitUi32(int n) { return 1Ui32 << n; }
constexpr uint64_t bitUi64(int n) { return 1Ui64 << n; }

constexpr uint64_t bit(int n) { return bitUi64(n); }
constexpr uint64_t pow2(int n) { return bitUi64(n); }

// Returns true if val is on the interval [a, b)
constexpr bool boundsCheck(int val, int a, int b) { return a <= val && val < b; }

#define EN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

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
  constexpr Angle()
    : Angle(0) {}

  constexpr explicit Angle(rad_t degrees)
    : m_Degrees(degrees) {}

  constexpr rad_t deg() const { return m_Degrees; }
  constexpr rad_t rad() const { return c_DegreeToRadFac * m_Degrees; }

  constexpr float degf() const { return static_cast<float>(deg()); }
  constexpr float radf() const { return static_cast<float>(rad()); }

  constexpr length_t degl() const { return static_cast<length_t>(deg()); }
  constexpr length_t radl() const { return static_cast<length_t>(rad()); }

  operator rad_t () = delete;
  operator rad_t& () = delete;

  constexpr bool operator>(Angle other) const { return m_Degrees > other.m_Degrees; }
  constexpr bool operator<(Angle other) const { return m_Degrees < other.m_Degrees; }
  constexpr bool operator>=(Angle other) const { return m_Degrees >= other.m_Degrees; }
  constexpr bool operator<=(Angle other) const { return m_Degrees <= other.m_Degrees; }
  constexpr bool operator==(Angle other) const { return m_Degrees == other.m_Degrees; }

  constexpr Angle operator-() const { return Angle(-m_Degrees); }

  constexpr Angle operator*(int n) const { return Angle(n * m_Degrees); }
  constexpr Angle operator*(float x) const { return Angle(static_cast<rad_t>(x) * m_Degrees); }
  constexpr Angle operator*(double x) const { return Angle(static_cast<rad_t>(x) * m_Degrees); }
  constexpr Angle operator*(Angle other) const { return Angle(m_Degrees * other.m_Degrees); }

  constexpr Angle operator/(int n) const { return Angle(m_Degrees / n); }
  constexpr Angle operator/(float x) const { return Angle(m_Degrees / static_cast<rad_t>(x)); }
  constexpr Angle operator/(double x) const { return Angle(m_Degrees / static_cast<rad_t>(x)); }

  constexpr Angle& operator+=(Angle other)
  {
    m_Degrees += other.m_Degrees;
    return *this;
  }

  constexpr Angle& operator-=(Angle other)
  {
    m_Degrees -= other.m_Degrees;
    return *this;
  }

  static constexpr Angle PI() { return Angle(180.0); }
  static constexpr Angle FromRad(float rad) { return Angle(c_RadToDegreeFac * static_cast<rad_t>(rad)); }
  static constexpr Angle FromRad(double rad) { return Angle(c_RadToDegreeFac * static_cast<rad_t>(rad)); }

private:
  rad_t m_Degrees;

  static constexpr rad_t c_DegreeToRadFac = static_cast<rad_t>(0.0174532925199432957692369076848861271344287188854L);
  static constexpr rad_t c_RadToDegreeFac = static_cast<rad_t>(57.295779513082320876798154814105170332405472466564L);
};

constexpr Angle operator*(int n, Angle theta) { return theta * n; }
constexpr Angle operator*(float x, Angle theta) { return theta * x; }
constexpr Angle operator*(double x, Angle theta) { return theta * x; }

// Angle literals
constexpr Angle operator"" _deg(uint64_t degrees) { return Angle(static_cast<rad_t>(degrees)); }
constexpr Angle operator"" _deg(long double degrees) { return Angle(static_cast<rad_t>(degrees)); }
constexpr Angle operator"" _rad(uint64_t radians) { return Angle::FromRad(static_cast<rad_t>(radians)); }
constexpr Angle operator"" _rad(long double radians) { return Angle::FromRad(static_cast<rad_t>(radians)); }



// ==================== Enabling Bitmasking for Enum Classes ==================== //
#define EN_ENABLE_BITMASK_OPERATORS(x)  \
template<>                              \
struct EnableBitMaskOperators<x> { static const bool enable = true; };

template<typename Enum>
struct EnableBitMaskOperators { static const bool enable = false; };

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
  Iterator end() { return Iterator(endEnum); }
  Iterator next() const
  {
    Iterator copy = *this;
    return ++copy;
  }

private:
  val_t value;
};