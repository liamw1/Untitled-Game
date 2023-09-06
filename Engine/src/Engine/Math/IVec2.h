#pragma once
#include "Engine/Utilities/Helpers.h"

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

  constexpr IntType& operator[](Axis axis) { return this->operator[][static_cast<int>(axis)]; }
  constexpr const IntType& operator[](Axis axis) const { return this->operator[][static_cast<int>(axis)]; }
  constexpr IntType& operator[](int index)
  {
    return const_cast<IntType&>(static_cast<const IVec2*>(this)->operator[](index));
  }
  constexpr const IntType& operator[](int index) const
  {
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, 0, 3), "Index is out of bounds!");
    return *(&i + index);
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
    EN_CORE_ASSERT(Engine::Debug::OverflowCheck(n, i) && Engine::Debug::OverflowCheck(n, j), "Integer overflow!");
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

  constexpr IVec2 operator+(IntType n) const { return Engine::Clone(*this) += n; }
  constexpr IVec2 operator-(IntType n) const { return Engine::Clone(*this) -= n; }

  constexpr IVec2 operator*(IntType n) const { return Engine::Clone(*this) *= n; }
  constexpr IVec2 operator/(IntType n) const { return Engine::Clone(*this) /= n; }

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



namespace std
{
  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const IVec2<IntType>& index)
  {
    return os << '[' << static_cast<int64_t>(index.i) << ", " << static_cast<int64_t>(index.j) << ']';
  }
}