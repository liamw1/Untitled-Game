#pragma once
#include "Direction.h"
#include "Basics.h"
#include "Vec.h"
#include "Engine/Debug/Assert.h"

namespace eng::math
{
  /*
    Represents a point on a 2D integer lattice.
  */
  template<std::integral T>
  struct IVec2
  {
    T i;
    T j;
  
    constexpr IVec2()
      : IVec2(0) {}
    constexpr IVec2(T n)
      : IVec2(n, n) {}
    constexpr IVec2(T _i, T _j)
      : i(_i), j(_j) {}
  
    explicit constexpr operator Vec2() const { return Vec2(i, j); }

    template<std::integral U>
    constexpr IVec2<U> upcast() const { return {arithmeticUpcast<U>(i), arithmeticUpcast<U>(j)}; }

    template<std::integral U>
    constexpr IVec2<U> checkedCast() const { return {arithmeticCast<U>(i), arithmeticCast<U>(j)}; }
  
    constexpr T& operator[](Axis axis) { ENG_MUTABLE_VERSION(operator[], axis); }
    constexpr const T& operator[](Axis axis) const
    {
      switch (axis)
      {
        case Axis::X: return i;
        case Axis::Y: return j;
      }
      throw std::invalid_argument("Invalid axis!");
    }
  
    // Define lexicographical ordering on 2D indices
    constexpr std::strong_ordering operator<=>(const IVec2& other) const = default;
  
    constexpr IVec2& operator+=(const IVec2& other)
    {
      i += other.i;
      j += other.j;
      return *this;
    }
    constexpr IVec2& operator-=(const IVec2& other) { return *this += -other; }
    constexpr IVec2& operator+=(T n) { return *this += IVec2(n); }
    constexpr IVec2& operator-=(T n) { return *this -= IVec2(n); }
  
    constexpr IVec2& operator*=(T n)
    {
      ENG_CORE_ASSERT(debug::productOverflowCheck(n, i) && debug::productOverflowCheck(n, j), "Integer overflow!");
      i *= n;
      j *= n;
      return *this;
    }
    constexpr IVec2& operator/=(T n)
    {
      i /= n;
      j /= n;
      return *this;
    }
  
    constexpr IVec2 operator-() const { return IVec2(-i, -j); };
  
    constexpr IVec2 operator+(IVec2 other) const { return other += *this; }
    constexpr IVec2 operator-(IVec2 other) const { return -(other -= *this); }
  
    constexpr IVec2 operator+(T n) const { return clone(*this) += n; }
    constexpr IVec2 operator-(T n) const { return clone(*this) -= n; }
  
    constexpr IVec2 operator*(T n) const { return clone(*this) *= n; }
    constexpr IVec2 operator/(T n) const { return clone(*this) /= n; }
  
    constexpr T l1Norm() const { return std::abs(i) + std::abs(j); }
    constexpr T dot(const IVec2& other) const { return i * other.i + j * other.j; }
  
    static constexpr IVec2 ToIndex(const Vec2& vec)
    {
      return IVec2(arithmeticCast<T>(std::floor(vec.x)), arithmeticCast<T>(std::floor(vec.y)));
    }
  };
  
  template<std::integral T>
  constexpr IVec2<T> operator*(T n, IVec2<T> index)
  {
    return index *= n;
  }
  
  template<std::integral T>
  constexpr IVec2<T> ComponentWiseMin(const IVec2<T>& a, const IVec2<T>& b)
  {
    return IVec2(std::min(a.i, b.i), std::min(a.j, b.j));
  }
  
  template<std::integral T>
  constexpr IVec2<T> ComponentWiseMax(const IVec2<T>& a, const IVec2<T>& b)
  {
    return IVec2(std::max(a.i, b.i), std::max(a.j, b.j));
  }
}



namespace std
{
  template<std::integral T>
  inline ostream& operator<<(ostream& os, const eng::math::IVec2<T>& index)
  {
    using promotedType = std::conditional_t<std::is_signed_v<T>, iMax, uMax>;
    return os << '[' << eng::arithmeticUpcast<promotedType>(index.i) << ", "
                     << eng::arithmeticUpcast<promotedType>(index.j) << ']';
  }

  template<std::integral T>
  struct hash<eng::math::IVec2<T>>
  {
    constexpr uSize operator()(const eng::math::IVec2<T>& index) const
    {
      return eng::u32Bit( 0) * eng::math::mod(index.i, eng::u32Bit(16)) +
             eng::u32Bit(16) * eng::math::mod(index.j, eng::u32Bit(16));
    }
  };
}