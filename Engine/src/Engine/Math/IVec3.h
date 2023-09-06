#pragma once
#include "Direction.h"
#include "IVec2.h"

template<std::integral IntType>
struct IVec3
{
  IntType i;
  IntType j;
  IntType k;

  constexpr IVec3()
    : IVec3(0) {}
  constexpr IVec3(IntType n)
    : IVec3(n, n, n) {}
  constexpr IVec3(IntType _i, IntType _j, IntType _k)
    : i(_i), j(_j), k(_k) {}
  constexpr IVec3(IVec2<IntType> intVec2, IntType _k)
    : i(intVec2.i), j(intVec2.j), k(_k) {}

  explicit constexpr operator Vec2() const { return Vec2(i, j); }
  explicit constexpr operator Vec3() const { return Vec3(i, j, k); }

  template<std::integral NewIntType>
  explicit constexpr operator IVec2<NewIntType>() const { return { static_cast<NewIntType>(i), static_cast<NewIntType>(j) }; }

  template<std::integral NewIntType>
  explicit constexpr operator IVec3<NewIntType>() const { return { static_cast<NewIntType>(i), static_cast<NewIntType>(j), static_cast<NewIntType>(k) }; }

  constexpr IntType& operator[](Axis axis) { return this->operator[](static_cast<int>(axis)); }
  constexpr const IntType& operator[](Axis axis) const { return this->operator[](static_cast<int>(axis)); }
  constexpr IntType& operator[](int index)
  {
    return const_cast<IntType&>(static_cast<const IVec3*>(this)->operator[](index));
  }
  constexpr const IntType& operator[](int index) const
  {
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(index, 0, 3), "Index is out of bounds!");
    return *(&i + index);
  }

  // Define lexicographical ordering on 3D indices
  constexpr std::strong_ordering operator<=>(const IVec3& other) const = default;

  constexpr IVec3& operator+=(const IVec3& other)
  {
    i += other.i;
    j += other.j;
    k += other.k;
    return *this;
  }
  constexpr IVec3& operator-=(const IVec3& other) { return *this += -other; }
  constexpr IVec3& operator+=(IntType n) { return *this += IVec3(n); }
  constexpr IVec3& operator-=(IntType n) { return *this -= IVec3(n); }

  constexpr IVec3& operator*=(IntType n)
  {
    EN_CORE_ASSERT(Engine::Debug::OverflowCheck(n, i) && Engine::Debug::OverflowCheck(n, j) && Engine::Debug::OverflowCheck(n, k), "Integer overflow!");
    i *= n;
    j *= n;
    k *= n;
    return *this;
  }
  constexpr IVec3& operator/=(IntType n)
  {
    i /= n;
    j /= n;
    k /= n;
    return *this;
  }

  constexpr IVec3 operator-() const { return IVec3(-i, -j, -k); };

  constexpr IVec3 operator+(IVec3 other) const { return other += *this; }
  constexpr IVec3 operator-(IVec3 other) const { return -(other -= *this); }

  constexpr IVec3 operator+(IntType n) const { return Engine::Clone(*this) += n; }
  constexpr IVec3 operator-(IntType n) const { return Engine::Clone(*this) -= n; }

  constexpr IVec3 operator*(IntType n) const { Engine::Clone(*this) *= n; }
  constexpr IVec3 operator/(IntType n) const { Engine::Clone(*this) /= n; }

  static constexpr IVec3 ToIndex(const Vec3& vec)
  {
    return IVec3(static_cast<IntType>(std::floor(vec.x)), static_cast<IntType>(std::floor(vec.y)), static_cast<IntType>(std::floor(vec.z)));
  }

  static constexpr IVec3 Dir(Direction direction)
  {
    static constexpr IVec3 directions[6] = { { -1, 0, 0}, { 1, 0, 0}, { 0, -1, 0}, { 0, 1, 0}, { 0, 0, -1}, { 0, 0, 1} };
                                        //       West        East        South        North       Bottom        Top

    return directions[static_cast<int>(direction)];
  }

  static constexpr IVec3 CreatePermuted(IntType i, IntType j, IntType k, Axis permutation)
  {
    return CreatePermuted(i, j, k, static_cast<int>(permutation));
  }
  static constexpr IVec3 CreatePermuted(IntType i, IntType j, IntType k, int permutation)
  {
    EN_CORE_ASSERT(Engine::Debug::BoundsCheck(permutation, 0, 3), "Not a valid permuation!");
    switch (permutation)
    {
      default:
      case 0: return IVec3(i, j, k);
      case 1: return IVec3(k, i, j);
      case 2: return IVec3(j, k, i);
    }
  }
};

template<std::integral IntType>
constexpr IVec3<IntType> operator*(IntType n, IVec3<IntType> index)
{
  return index *= n;
}

template<std::integral IntType>
constexpr IVec3<IntType> ComponentWiseMin(const IVec3<IntType>& a, const IVec3<IntType>& b)
{
  return IVec3(std::min(a.i, b.i), std::min(a.j, b.j), std::min(a.k, b.k));
}

template<std::integral IntType>
constexpr IVec3<IntType> ComponentWiseMax(const IVec3<IntType>& a, const IVec3<IntType>& b)
{
  return IVec3(std::max(a.i, b.i), std::max(a.j, b.j), std::max(a.k, b.k));
}



namespace std
{
  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const IVec3<IntType>& index)
  {
    return os << '[' << static_cast<int64_t>(index.i) << ", " << static_cast<int64_t>(index.j) << ", " << static_cast<int64_t>(index.k) << ']';
  }
}