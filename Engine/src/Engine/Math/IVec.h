#pragma once
#include "Direction.h"
#include "Engine/Core/Concepts.h"

template<std::integral IntType>
struct IVec2
{
  IntType i;
  IntType j;

  constexpr IVec2()
    : IVec2(0, 0) {}
  constexpr IVec2(IntType n)
    : i(n), j(n) {}
  constexpr IVec2(IntType _i, IntType _j)
    : i(_i), j(_j) {}

  explicit constexpr operator Vec2() const { return { i, j }; }

  template<std::integral NewIntType>
  explicit constexpr operator IVec2<NewIntType>() const { return { static_cast<NewIntType>(i), static_cast<NewIntType>(j) }; }

  constexpr IntType& operator[](int index)
  {
    return const_cast<IntType&>(static_cast<const IVec2*>(this)->operator[](index));
  }

  constexpr const IntType& operator[](int index) const
  {
    EN_CORE_ASSERT(boundsCheck(index, 0, 3), "Index is out of bounds!");
    return *(&i + index);
  }

  // Define lexicographical ordering on 2D indices
  constexpr std::strong_ordering operator<=>(const IVec2<IntType>& other) const = default;

  constexpr IVec2<IntType> operator-() const { return IVec2<IntType>(-i, -j); };
  constexpr IVec2<IntType> operator+=(const IVec2<IntType>& other)
  {
    i += other.i;
    j += other.j;
    return *this;
  }
  constexpr IVec2<IntType> operator-=(const IVec2<IntType>& other)
  {
    i -= other.i;
    j -= other.j;
    return *this;
  }
  constexpr IVec2<IntType> operator*=(IntType n)
  {
    EN_CORE_ASSERT(n == 0 || (static_cast<IntType>(i * n) / n == i && static_cast<IntType>(j * n) / n == j), "Integer overflow!");
    i *= n;
    j *= n;
    return *this;
  }
  constexpr IVec2<IntType> operator/=(IntType n)
  {
    i /= n;
    j /= n;
    return *this;
  }

  constexpr IVec2<IntType> operator+(IVec2<IntType> other) const { return other += *this; }
  constexpr IVec2<IntType> operator-(IVec2<IntType> other) const { return -(other -= *this); }

  constexpr IVec2<IntType> operator+(IntType n) const { return IVec2<IntType>(n, n) += *this; }
  constexpr IVec2<IntType> operator-(IntType n) const { return -(IVec2<IntType>(n, n) -= *this); }

  constexpr IVec2<IntType> operator*(IntType n) const
  {
    IVec2<IntType> copy = *this;
    return copy *= n;
  }
  constexpr IVec2<IntType> operator/(IntType n) const
  {
    IVec2<IntType> copy = *this;
    return copy /= n;
  }

  static constexpr IVec2<IntType> ToIndex(const Vec2& vec)
  {
    return { static_cast<IntType>(std::floor(vec.x)), static_cast<IntType>(std::floor(vec.y)) };
  }
};

template<std::integral IntType>
struct IVec3
{
  IntType i;
  IntType j;
  IntType k;

  constexpr IVec3()
    : IVec3(0, 0, 0) {}
  constexpr IVec3(IntType n)
    : i(n), j(n), k(n) {}
  constexpr IVec3(IntType _i, IntType _j, IntType _k)
    : i(_i), j(_j), k(_k) {}
  constexpr IVec3(IVec2<IntType> intVec2, IntType _k)
    : i(intVec2.i), j(intVec2.j), k(_k) {}

  explicit constexpr operator Vec2() const { return { i, j }; }
  explicit constexpr operator Vec3() const { return { i, j, k }; }
  explicit constexpr operator IVec2<IntType>() const { return { i, j }; }

  template<std::integral NewIntType>
  explicit constexpr operator IVec3<NewIntType>() const { return { static_cast<NewIntType>(i), static_cast<NewIntType>(j), static_cast<NewIntType>(k) }; }

  constexpr IntType& operator[](int index)
  {
    return const_cast<IntType&>(static_cast<const IVec3*>(this)->operator[](index));
  }

  constexpr const IntType& operator[](int index) const
  {
    EN_CORE_ASSERT(boundsCheck(index, 0, 3), "Index is out of bounds!");
    return *(&i + index);
  }

  // Define lexicographical ordering on 3D indices
  constexpr std::strong_ordering operator<=>(const IVec3<IntType>& other) const = default;

  constexpr IVec3<IntType> operator-() const { return IVec3<IntType>(-i, -j, -k); };
  constexpr IVec3<IntType> operator+=(const IVec3<IntType>& other)
  {
    i += other.i;
    j += other.j;
    k += other.k;
    return *this;
  }
  constexpr IVec3<IntType> operator-=(const IVec3<IntType>& other)
  {
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return *this;
  }
  constexpr IVec3<IntType> operator*=(IntType n)
  {
    EN_CORE_ASSERT(n == 0 || (static_cast<IntType>(i * n) / n == i && static_cast<IntType>(j * n) / n == j && static_cast<IntType>(k * n) / n == k), "Integer overflow!");
    i *= n;
    j *= n;
    k *= n;
    return *this;
  }
  constexpr IVec3<IntType> operator/=(IntType n)
  {
    i /= n;
    j /= n;
    k /= n;
    return *this;
  }

  constexpr IVec3<IntType> operator+(IVec3<IntType> other) const { return other += *this; }
  constexpr IVec3<IntType> operator-(IVec3<IntType> other) const { return -(other -= *this); }

  constexpr IVec3<IntType> operator+(IntType n) const { return IVec3<IntType>(n, n, n) += *this; }
  constexpr IVec3<IntType> operator-(IntType n) const { return -(IVec3<IntType>(n, n, n) -= *this); }

  constexpr IVec3<IntType> operator*(IntType n) const
  {
    IVec3<IntType> copy = *this;
    return copy *= n;
  }
  constexpr IVec3<IntType> operator/(IntType n) const
  {
    IVec3<IntType> copy = *this;
    return copy /= n;
  }

  static constexpr IVec3<IntType> ToIndex(const Vec3& vec)
  {
    return { static_cast<IntType>(std::floor(vec.x)), static_cast<IntType>(std::floor(vec.y)), static_cast<IntType>(std::floor(vec.z)) };
  }

  static constexpr IVec3<IntType> Dir(Direction direction)
  {
    static constexpr IVec3<IntType> directions[6] = { { -1, 0, 0}, { 1, 0, 0}, { 0, -1, 0}, { 0, 1, 0}, { 0, 0, -1}, { 0, 0, 1} };
                                                   //       West        East        South        North       Bottom        Top

    return directions[static_cast<int>(direction)];
  }

  static constexpr IVec3<IntType> CreatePermuted(IntType i, IntType j, IntType k, int permutation)
  {
    EN_CORE_ASSERT(0 <= permutation && permutation < 3, "Not a valid permuation!");
    switch (permutation)
    {
      default:
      case 0: return IVec3<IntType>(i, j, k);
      case 1: return IVec3<IntType>(k, i, j);
      case 2: return IVec3<IntType>(j, k, i);
    }
  }
};

template<std::integral IntType>
constexpr IVec3<IntType> operator*(IntType n, IVec3<IntType> index)
{
  return index *= n;
}

template<std::integral IntType>
constexpr IVec3<IntType> componentWiseMin(const IVec3<IntType>& a, const IVec3<IntType>& b)
{
  return IVec3<IntType>(std::min(a.i, b.i), std::min(a.j, b.j), std::min(a.k, b.k));
}

template<std::integral IntType>
constexpr IVec3<IntType> componentWiseMax(const IVec3<IntType>& a, const IVec3<IntType>& b)
{
  return IVec3<IntType>(std::max(a.i, b.i), std::max(a.j, b.j), std::max(a.k, b.k));
}




namespace std
{
  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const IVec2<IntType>& index)
  {
    return os << '[' << static_cast<int64_t>(index.i) << ", " << static_cast<int64_t>(index.j) << ']';
  }

  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const IVec3<IntType>& index)
  {
    return os << '[' << static_cast<int64_t>(index.i) << ", " << static_cast<int64_t>(index.j) << ", " << static_cast<int64_t>(index.k) << ']';
  }
}
