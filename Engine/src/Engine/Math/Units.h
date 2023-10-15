#pragma once

using length_t = float;
using rad_t = float;
using seconds = float;

namespace eng::math
{
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
  
    constexpr std::partial_ordering operator<=>(const Angle& other) const = default;
  
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
}

// Literals
constexpr length_t operator"" _m(long double x) { return static_cast<length_t>(x); }
constexpr length_t operator"" _m(uint64_t x) { return static_cast<length_t>(x); }

constexpr eng::math::Angle operator"" _deg(uint64_t degrees) { return eng::math::Angle(static_cast<rad_t>(degrees)); }
constexpr eng::math::Angle operator"" _deg(long double degrees) { return eng::math::Angle(static_cast<rad_t>(degrees)); }
constexpr eng::math::Angle operator"" _rad(uint64_t radians) { return eng::math::Angle::FromRad(static_cast<rad_t>(radians)); }
constexpr eng::math::Angle operator"" _rad(long double radians) { return eng::math::Angle::FromRad(static_cast<rad_t>(radians)); }