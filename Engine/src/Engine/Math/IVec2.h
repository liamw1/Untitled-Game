#pragma once
#include "Direction.h"
#include "Engine/Utilities/Helpers.h"
#include "Engine/Utilities/BitUtilities.h"

namespace eng::math
{
  /*
    Represents a point on a 2D integer lattice.
  */
  template<std::integral IntType>
  struct IVec2
  {
    IntType i;
    IntType j;
  
    constexpr IVec2()
      : IVec2(0) {}
    constexpr IVec2(IntType n)
      : IVec2(n, n) {}
    constexpr IVec2(IntType _i, IntType _j)
      : i(_i), j(_j) {}
  
    explicit constexpr operator Vec2() const { return Vec2(i, j); }
  
    template<std::integral NewIntType>
    explicit constexpr operator IVec2<NewIntType>() const { return { static_cast<NewIntType>(i), static_cast<NewIntType>(j) }; }
  
    constexpr IntType& operator[](Axis axis)
    {
      return const_cast<IntType&>(static_cast<const IVec2*>(this)->operator[](axis));
    }
    constexpr const IntType& operator[](Axis axis) const
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
    constexpr IVec2& operator+=(IntType n) { return *this += IVec2(n); }
    constexpr IVec2& operator-=(IntType n) { return *this -= IVec2(n); }
  
    constexpr IVec2& operator*=(IntType n)
    {
      EN_CORE_ASSERT(debug::OverflowCheck(n, i) && debug::OverflowCheck(n, j), "Integer overflow!");
      i *= n;
      j *= n;
      return *this;
    }
    constexpr IVec2& operator/=(IntType n)
    {
      i /= n;
      j /= n;
      return *this;
    }
  
    constexpr IVec2 operator-() const { return IVec2(-i, -j); };
  
    constexpr IVec2 operator+(IVec2 other) const { return other += *this; }
    constexpr IVec2 operator-(IVec2 other) const { return -(other -= *this); }
  
    constexpr IVec2 operator+(IntType n) const { return Clone(*this) += n; }
    constexpr IVec2 operator-(IntType n) const { return Clone(*this) -= n; }
  
    constexpr IVec2 operator*(IntType n) const { return Clone(*this) *= n; }
    constexpr IVec2 operator/(IntType n) const { return Clone(*this) /= n; }
  
    constexpr int l1Norm() const { return std::abs(i) + std::abs(j); }
    constexpr int dot(const IVec2& other) const { return i * other.i + j * other.j; }
  
    static constexpr IVec2 ToIndex(const Vec2& vec)
    {
      return IVec2(static_cast<IntType>(std::floor(vec.x)), static_cast<IntType>(std::floor(vec.y)));
    }
  };
  
  template<std::integral IntType>
  constexpr IVec2<IntType> operator*(IntType n, IVec2<IntType> index)
  {
    return index *= n;
  }
  
  template<std::integral IntType>
  constexpr IVec2<IntType> ComponentWiseMin(const IVec2<IntType>& a, const IVec2<IntType>& b)
  {
    return IVec2(std::min(a.i, b.i), std::min(a.j, b.j));
  }
  
  template<std::integral IntType>
  constexpr IVec2<IntType> ComponentWiseMax(const IVec2<IntType>& a, const IVec2<IntType>& b)
  {
    return IVec2(std::max(a.i, b.i), std::max(a.j, b.j));
  }
}



namespace std
{
  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const eng::math::IVec2<IntType>& index)
  {
    return os << '[' << static_cast<int64_t>(index.i) << ", " << static_cast<int64_t>(index.j) << ']';
  }

  template<std::integral IntType>
  struct hash<eng::math::IVec2<IntType>>
  {
    constexpr int operator()(const eng::math::IVec2<IntType>& index) const
    {
      return eng::u32Bit( 0) * (index.i % eng::u32Bit(16)) +
             eng::u32Bit(16) * (index.j % eng::u32Bit(16));
    }
  };
}