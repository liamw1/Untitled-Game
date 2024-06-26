#pragma once
#include "Direction.h"
#include "Basics.h"
#include "Vec.h"
#include "Engine/Debug/Assert.h"
#include "Engine/Utilities/EnumUtilities.h"

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
    constexpr IVec2(EnumBitMask<Axis> bitMask)
      : IVec2(bitMask[Axis::X], bitMask[Axis::Y]) {}
  
    explicit constexpr operator Vec2() const { return Vec2(i, j); }

    template<std::integral U>
    constexpr IVec2<U> upcast() const { return { arithmeticUpcast<U>(i), arithmeticUpcast<U>(j) }; }

    template<std::integral U>
    constexpr IVec2<U> checkedCast() const { return { arithmeticCast<U>(i), arithmeticCast<U>(j) }; }

    template<std::integral U>
    constexpr IVec2<U> uncheckedCast() const { return { arithmeticCastUnchecked<U>(i), arithmeticCastUnchecked<U>(j) }; }
  
    constexpr T& operator[](Axis axis) { ENG_MUTABLE_VERSION(operator[], axis); }
    constexpr const T& operator[](Axis axis) const
    {
      switch (axis)
      {
        case Axis::X: return i;
        case Axis::Y: return j;
      }
      throw CoreException("Invalid axis!");
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

    constexpr bool nonNegative() const { return i >= 0 && j >= 0; }

    constexpr IVec2& flooredDivide(T n)
    {
      i = flooredDiv(i, n);
      j = flooredDiv(j, n);
      return *this;
    }
  
    static constexpr IVec2 ToIndex(const Vec2& vec)
    {
      return IVec2(arithmeticCast<T>(std::floor(vec.x)), arithmeticCast<T>(std::floor(vec.y)));
    }
  };
  
  template<std::integral U, std::integral T>
  constexpr IVec2<T> operator*(U n, IVec2<T> index)
  {
    return index *= static_cast<T>(n);
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
      constexpr i32 n = std::numeric_limits<uSize>::digits / 2;
      constexpr uSize stride = eng::math::pow2<uSize>(n) - 1;
      constexpr eng::math::IVec2<uSize> strides(1, stride);

      return strides.i * eng::math::mod<stride>(index.i) +
             strides.j * eng::math::mod<stride>(index.j);
    }
  };
}