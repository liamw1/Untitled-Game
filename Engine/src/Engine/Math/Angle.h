#pragma once
#include "Engine/Core/Units.h"
#include "Engine/Utilities/Helpers.h"

namespace eng::math
{
  class Angle
  {
    f64 m_Degrees;

  public:
    constexpr Angle()
      : Angle(0) {}

    template<Arithmetic T>
    constexpr explicit Angle(T degrees)
      : m_Degrees(arithmeticUpcast<f64>(degrees)) {}

    operator f64() = delete;
    operator f64& () = delete;
  
    constexpr f64 deg() const { return m_Degrees; }
    constexpr f64 rad() const { return ConvertToRad(m_Degrees); }
  
    constexpr f32 degf() const { return arithmeticCastUnchecked<f32>(deg()); }
    constexpr f32 radf() const { return arithmeticCastUnchecked<f32>(rad()); }
  
    constexpr length_t degl() const { return arithmeticCastUnchecked<length_t>(deg()); }
    constexpr length_t radl() const { return arithmeticCastUnchecked<length_t>(rad()); }
  
    constexpr std::partial_ordering operator<=>(const Angle& other) const = default;
  
    constexpr Angle operator-() const { return Angle(-m_Degrees); }

    template<Arithmetic T>
    constexpr Angle operator*(T x) { return *this * Angle(x); }
    constexpr Angle operator*(Angle other) { return clone(*this) *= other; }

    template<Arithmetic T>
    constexpr Angle operator/(T x) { return *this / Angle(x); }
    constexpr Angle operator/(Angle other) { return clone(*this) /= other; }

    template<Arithmetic T>
    constexpr Angle operator+(T x) { return *this + Angle(x); }
    constexpr Angle operator+(Angle other) { return clone(*this) += other; }

    template<Arithmetic T>
    constexpr Angle operator-(T x) { return *this - Angle(x); }
    constexpr Angle operator-(Angle other) { return clone(*this) -= other; }

    template<Arithmetic T>
    constexpr Angle& operator*=(T x) { return *this *= Angle(x); }
    constexpr Angle& operator*=(Angle other)
    {
      m_Degrees *= other.m_Degrees;
      return *this;
    }

    template<Arithmetic T>
    constexpr Angle& operator/=(T x) { return *this /= Angle(x); }
    constexpr Angle& operator/=(Angle other)
    {
      m_Degrees /= other.m_Degrees;
      return *this;
    }

    template<Arithmetic T>
    constexpr Angle& operator+=(T x) { return *this += Angle(x); }
    constexpr Angle& operator+=(Angle other)
    {
      m_Degrees += other.m_Degrees;
      return *this;
    }

    template<Arithmetic T>
    constexpr Angle& operator-=(T x) { return *this -= Angle(x); }
    constexpr Angle& operator-=(Angle other) { return *this += -other; }

    template<Arithmetic T>
    static constexpr Angle FromRad(T rad) { return Angle(ConverToDegrees(arithmeticUpcast<f64>(rad))); }
    static constexpr Angle PI() { return Angle(180.0); }
  
  private:
    static constexpr f64 ConvertToRad(f64 degrees) { return std::numbers::pi_v<f64> * degrees / 180; }
    static constexpr f64 ConverToDegrees(f64 radians) { return 180 * radians / std::numbers::pi_v<f64>; }
  };

  template<Arithmetic T>
  constexpr Angle operator*(T x, Angle theta) { return theta * x; }
}

// Literals
constexpr eng::math::Angle operator"" _deg(uSize degrees) { return eng::math::Angle(degrees); }
constexpr eng::math::Angle operator"" _deg(fMax degrees) { return eng::math::Angle(degrees); }
constexpr eng::math::Angle operator"" _rad(uSize radians) { return eng::math::Angle::FromRad(radians); }
constexpr eng::math::Angle operator"" _rad(fMax radians) { return eng::math::Angle::FromRad(radians); }