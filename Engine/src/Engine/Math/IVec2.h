#pragma once
#include "Direction.h"
#include "Engine/Core/Concepts.h"
#include "Engine/Utilities/Helpers.h"

namespace eng::math
{
  /*
    Represents a point on a 2D integer lattice.
  */
  template<Integer T>
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
  
    template<Integer U>
    explicit constexpr operator IVec2<U>() const { return { static_cast<U>(i), static_cast<U>(j) }; }
  
    constexpr T& operator[](Axis axis) { return ENG_MUTABLE_VERSION(operator[], axis); }
    constexpr const T& operator[](Axis axis) const
    {
      switch (axis)
      {
        case Axis::X: return i;
        case Axis::Y: return j;
        default:      throw std::invalid_argument("Invalid axis!");
      }
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
      ENG_CORE_ASSERT(debug::OverflowCheck(n, i) && debug::OverflowCheck(n, j), "Integer overflow!");
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
  
    constexpr int l1Norm() const { return std::abs(i) + std::abs(j); }
    constexpr int dot(const IVec2& other) const { return i * other.i + j * other.j; }
  
    static constexpr IVec2 ToIndex(const Vec2& vec)
    {
      return IVec2(static_cast<T>(std::floor(vec.x)), static_cast<T>(std::floor(vec.y)));
    }
  };
  
  template<Integer T>
  constexpr IVec2<T> operator*(T n, IVec2<T> index)
  {
    return index *= n;
  }
  
  template<Integer T>
  constexpr IVec2<T> ComponentWiseMin(const IVec2<T>& a, const IVec2<T>& b)
  {
    return IVec2(std::min(a.i, b.i), std::min(a.j, b.j));
  }
  
  template<Integer T>
  constexpr IVec2<T> ComponentWiseMax(const IVec2<T>& a, const IVec2<T>& b)
  {
    return IVec2(std::max(a.i, b.i), std::max(a.j, b.j));
  }
}



namespace std
{
  template<eng::Integer T>
  inline ostream& operator<<(ostream& os, const eng::math::IVec2<T>& index)
  {
    return os << '[' << static_cast<int64_t>(index.i) << ", " << static_cast<int64_t>(index.j) << ']';
  }

  template<eng::Integer T>
  struct hash<eng::math::IVec2<T>>
  {
    constexpr int operator()(const eng::math::IVec2<T>& index) const
    {
      return eng::u32Bit( 0) * (index.i % eng::u32Bit(16)) +
             eng::u32Bit(16) * (index.j % eng::u32Bit(16));
    }
  };
}