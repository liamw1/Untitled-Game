#pragma once
#include "Engine/Utilities/BitUtilities.h"
#include "Engine/Utilities/EnumUtilities.h"

enum class Direction : int
{
  West, East, South, North, Bottom, Top,
  Null,

  Begin = 0, End = Null
};
using Directions = Iterator<Direction, Direction::Begin, Direction::End>;

/*
  \returns The direction opposite the given direction.
*/
constexpr Direction operator!(const Direction& direction)
{
  int directionID = static_cast<int>(direction);
  Direction oppositeDirection = static_cast<Direction>(directionID % 2 ? directionID - 1 : directionID + 1);
  return oppositeDirection;
}

constexpr bool IsUpstream(Direction direction) { return static_cast<int>(direction) % 2; }
constexpr int GetCoordID(Direction direction) { return static_cast<int>(direction) / 2; }



template<typename T>
concept IntegerType = std::is_integral_v<T>;

template<IntegerType IntType>
struct Index2D
{
  IntType i;
  IntType j;

  constexpr Index2D()
    : Index2D(0, 0) {}
  constexpr Index2D(IntType _i, IntType _j)
    : i(_i), j(_j) {}

  constexpr IntType& operator[](int index)
  {
    return const_cast<IntType&>(static_cast<const Index2D*>(this)->operator[](index));
  }

  constexpr const IntType& operator[](int index) const
  {
    EN_ASSERT(boundsCheck(index, 0, 3), "Index is out of bounds!");
    return *(&i + index);
  }

  // Comparison operators define lexicographical ordering on 3D indices
  constexpr bool operator==(const Index2D<IntType>& other) const { return i == other.i && j == other.j; }
  constexpr bool operator!=(const Index2D<IntType>& other) const { return !(*this == other); }
  constexpr bool operator<(const Index2D<IntType>& other) const { return i < other.i || (i == other.i && j < other.j); }
  constexpr bool operator>(const Index2D<IntType>& other) const { return i > other.i || (i == other.i && j > other.j); }
  constexpr bool operator<=(const Index2D<IntType>& other) const { return !(*this > other); }
  constexpr bool operator>=(const Index2D<IntType>& other) const { return !(*this < other); }

  constexpr Index2D<IntType> operator-() const { return Index2D<IntType>(-i, -j); };
  constexpr Index2D<IntType> operator+=(const Index2D<IntType>& other)
  {
    i += other.i;
    j += other.j;
    return *this;
  }
  constexpr Index2D<IntType> operator-=(const Index2D<IntType>& other)
  {
    i -= other.i;
    j -= other.j;
    return *this;
  }
  constexpr Index2D<IntType> operator*=(IntType n)
  {
    EN_ASSERT(n == 0 || (static_cast<IntType>(i * n) / n == i && static_cast<IntType>(j * n) / n == j), "Integer overflow!");
    i *= n;
    j *= n;
    return *this;
  }
  constexpr Index2D<IntType> operator/=(IntType n)
  {
    i /= n;
    j /= n;
    return *this;
  }

  constexpr Index2D<IntType> operator+(Index2D<IntType> other) const { return other += *this; }
  constexpr Index2D<IntType> operator-(Index2D<IntType> other) const { return -(other -= *this); }

  constexpr Index2D<IntType> operator+(IntType n) const { return Index2D<IntType>(n, n) += *this; }
  constexpr Index2D<IntType> operator-(IntType n) const { return -(Index2D<IntType>(n, n) -= *this); }

  constexpr Index2D<IntType> operator*(IntType n) const
  {
    Index2D<IntType> copy = *this;
    return copy *= n;
  }
  constexpr Index2D<IntType> operator/(IntType n) const
  {
    Index2D<IntType> copy = *this;
    return copy /= n;
  }

  explicit constexpr operator Vec2() const { return { i, j }; }

  static constexpr Index2D<IntType> ToIndex(const Vec2& vec)
  {
    return { static_cast<IntType>(std::floor(vec.x)), static_cast<IntType>(std::floor(vec.y)) };
  }
};

/*
  Structs for indexing of blocks and chunks.

  BlockIndex indexes blocks within a chunk
  LocalIndex indexes chunks relative to the origin chunk (usually the chunk the player is in)
  GlobalIndex uniquely identifies a chunk within worldspace
*/
template<IntegerType IntType>
struct Index3D
{
  IntType i;
  IntType j;
  IntType k;

  constexpr Index3D()
    : Index3D(0, 0, 0) {}
  constexpr Index3D(IntType _i, IntType _j, IntType _k)
    : i(_i), j(_j), k(_k) {}
  constexpr Index3D(Index2D<IntType> index2D, IntType _k)
    : i(index2D.i), j(index2D.j), k(_k) {}

  constexpr IntType& operator[](int index)
  {
    return const_cast<IntType&>(static_cast<const Index3D*>(this)->operator[](index));
  }

  constexpr const IntType& operator[](int index) const
  {
    EN_ASSERT(boundsCheck(index, 0, 3), "Index is out of bounds!");
    return *(&i + index);
  }

  // Comparison operators define lexicographical ordering on 3D indices
  constexpr bool operator==(const Index3D<IntType>& other) const { return i == other.i && j == other.j && k == other.k; }
  constexpr bool operator!=(const Index3D<IntType>& other) const { return !(*this == other); }
  constexpr bool operator<(const Index3D<IntType>& other) const { return i < other.i || (i == other.i && j < other.j) || (i == other.i && j == other.j && k < other.k); }
  constexpr bool operator>(const Index3D<IntType>& other) const { return i > other.i || (i == other.i && j > other.j) || (i == other.i && j == other.j && k > other.k); }
  constexpr bool operator<=(const Index3D<IntType>& other) const { return !(*this > other); }
  constexpr bool operator>=(const Index3D<IntType>& other) const { return !(*this < other); }

  constexpr Index3D<IntType> operator-() const { return Index3D<IntType>(-i, -j, -k); };
  constexpr Index3D<IntType> operator+=(const Index3D<IntType>& other)
  {
    i += other.i;
    j += other.j;
    k += other.k;
    return *this;
  }
  constexpr Index3D<IntType> operator-=(const Index3D<IntType>& other)
  {
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return *this;
  }
  constexpr Index3D<IntType> operator*=(IntType n)
  {
    EN_ASSERT(n == 0 || (static_cast<IntType>(i * n) / n == i && static_cast<IntType>(j * n) / n == j && static_cast<IntType>(k * n) / n == k), "Integer overflow!");
    i *= n;
    j *= n;
    k *= n;
    return *this;
  }
  constexpr Index3D<IntType> operator/=(IntType n)
  {
    i /= n;
    j /= n;
    k /= n;
    return *this;
  }

  constexpr Index3D<IntType> operator+(Index3D<IntType> other) const { return other += *this; }
  constexpr Index3D<IntType> operator-(Index3D<IntType> other) const { return -(other -= *this); }

  constexpr Index3D<IntType> operator+(IntType n) const { return Index3D<IntType>(n, n, n) += *this; }
  constexpr Index3D<IntType> operator-(IntType n) const { return -(Index3D<IntType>(n, n, n) -= *this); }

  constexpr Index3D<IntType> operator*(IntType n) const
  {
    Index3D<IntType> copy = *this;
    return copy *= n;
  }
  constexpr Index3D<IntType> operator/(IntType n) const
  {
    Index3D<IntType> copy = *this;
    return copy /= n;
  }

  explicit constexpr operator Vec2() const { return { i, j }; }
  explicit constexpr operator Vec3() const { return { i, j, k }; }
  explicit constexpr operator Index2D<IntType>() const { return { i, j }; }

  static constexpr Index3D<IntType> ToIndex(const Vec3& vec)
  {
    return { static_cast<IntType>(std::floor(vec.x)), static_cast<IntType>(std::floor(vec.y)), static_cast<IntType>(std::floor(vec.z)) };
  }

  static constexpr Index3D<IntType> Dir(Direction direction)
  {
    static constexpr Index3D<IntType> directions[6] = { { -1, 0, 0}, { 1, 0, 0}, { 0, -1, 0}, { 0, 1, 0}, { 0, 0, -1}, { 0, 0, 1} };
                                                   //       West        East        South        North       Bottom        Top

    return directions[static_cast<int>(direction)];
  }

  static constexpr Index3D<IntType> CreatePermuted(IntType i, IntType j, IntType k, int permutation)
  {
    EN_ASSERT(0 <= permutation && permutation < 3, "Not a valid permuation!");
    switch (permutation)
    {
      default:
      case 0: return Index3D<IntType>(i, j, k);
      case 1: return Index3D<IntType>(k, i, j);
      case 2: return Index3D<IntType>(j, k, i);
    }
  }
};

template<IntegerType IntType>
constexpr Index3D<IntType> operator*(IntType n, Index3D<IntType> index)
{
  return index *= n;
}



// =========== Precision selection for Indices ============= //

template <bool isDoublePrecision> struct GlobalIndexSelector;

template<> struct GlobalIndexSelector<true> { using type = typename int64_t; };
template<> struct GlobalIndexSelector<false> { using type = typename int32_t; };

using blockIndex_t = int8_t;
using localIndex_t = int16_t;
using globalIndex_t = typename GlobalIndexSelector<std::is_same<double, length_t>::value>::type;

using SurfaceMapIndex = Index2D<globalIndex_t>;
using BlockIndex = Index3D<blockIndex_t>;
using LocalIndex = Index3D<localIndex_t>;
using GlobalIndex = Index3D<globalIndex_t>;



namespace std
{
  template<IntegerType IntType>
  inline ostream& operator<<(ostream& os, const Index2D<IntType>& index)
  {
    return os << '[' << index.i << ", " << index.j << ']';
  }

  template<IntegerType IntType>
  inline ostream& operator<<(ostream& os, const Index3D<IntType>& index)
  {
    return os << '[' << index.i << ", " << index.j << ", " << index.k << ']';
  }

  template<>
  struct hash<SurfaceMapIndex>
  {
    int operator()(const SurfaceMapIndex& index) const
    {
      return index.i % bitUi32(16) + bitUi32(16) * (index.j % bitUi32(16));
    }
  };

  template<>
  struct hash<GlobalIndex>
  {
    int operator()(const GlobalIndex& index) const
    {
      return index.i % bitUi32(10) + bitUi32(10) * (index.j % bitUi32(10)) + bitUi32(20) * (index.k % bitUi32(10));
    }
  };
}