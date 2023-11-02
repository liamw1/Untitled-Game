#pragma once
#include "Engine/Core/Casting.h"

namespace eng::math
{
  class Angle
  {
    rad_t m_Degrees;

  public:
    constexpr Angle()
      : Angle(0) {}
  
    constexpr explicit Angle(rad_t degrees)
      : m_Degrees(degrees) {}
  
    constexpr rad_t deg() const { return m_Degrees; }
    constexpr rad_t rad() const { return c_DegreeToRadConversion * m_Degrees; }
  
    constexpr f32 degf() const { return arithmeticCastUnchecked<f32>(deg()); }
    constexpr f32 radf() const { return arithmeticCastUnchecked<f32>(rad()); }
  
    constexpr length_t degl() const { return arithmeticCastUnchecked<length_t>(deg()); }
    constexpr length_t radl() const { return arithmeticCastUnchecked<length_t>(rad()); }
  
    operator rad_t() = delete;
    operator rad_t&() = delete;
  
    constexpr std::partial_ordering operator<=>(const Angle& other) const = default;
  
    constexpr Angle operator-() const { return Angle(-m_Degrees); }
  
    constexpr Angle operator*(i32 n) const { return Angle(n * m_Degrees); }
    constexpr Angle operator*(f32 x) const { return Angle(arithmeticUpcast<rad_t>(x) * m_Degrees); }
    constexpr Angle operator*(f64 x) const { return Angle(arithmeticCastUnchecked<rad_t>(x) * m_Degrees); }
    constexpr Angle operator*(Angle other) const { return Angle(m_Degrees * other.m_Degrees); }
  
    constexpr Angle operator/(i32 n) const { return Angle(m_Degrees / n); }
    constexpr Angle operator/(f32 x) const { return Angle(m_Degrees / arithmeticUpcast<rad_t>(x)); }
    constexpr Angle operator/(f64 x) const { return Angle(m_Degrees / arithmeticCastUnchecked<rad_t>(x)); }
  
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
    static constexpr Angle FromRad(f32 rad) { return Angle(c_RadToDegreeConversion * arithmeticUpcast<rad_t>(rad)); }
    static constexpr Angle FromRad(f64 rad) { return Angle(c_RadToDegreeConversion * arithmeticCastUnchecked<rad_t>(rad)); }
  
  private:
    static constexpr rad_t c_DegreeToRadConversion = std::numbers::pi_v<rad_t> / 180;
    static constexpr rad_t c_RadToDegreeConversion = 180 / std::numbers::pi_v<rad_t>;
  };

  constexpr Angle operator*(i32 n, Angle theta) { return theta * n; }
  constexpr Angle operator*(f32 x, Angle theta) { return theta * x; }
  constexpr Angle operator*(f64 x, Angle theta) { return theta * x; }
}

// Literals
constexpr length_t operator"" _m(u64 x) { return eng::arithmeticUpcast<length_t>(x); }
constexpr length_t operator"" _m(fMax x) { return eng::arithmeticCastUnchecked<length_t>(x); }

constexpr eng::math::Angle operator"" _deg(u64 degrees) { return eng::math::Angle(eng::arithmeticUpcast<rad_t>(degrees)); }
constexpr eng::math::Angle operator"" _deg(fMax degrees) { return eng::math::Angle(eng::arithmeticCastUnchecked<rad_t>(degrees)); }
constexpr eng::math::Angle operator"" _rad(u64 radians) { return eng::math::Angle::FromRad(eng::arithmeticUpcast<rad_t>(radians)); }
constexpr eng::math::Angle operator"" _rad(fMax radians) { return eng::math::Angle::FromRad(eng::arithmeticCastUnchecked<rad_t>(radians)); }