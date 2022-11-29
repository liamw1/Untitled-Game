#pragma once
#include "Block/Block.h"

template<typename intType>
struct Index2D
{
  intType i;
  intType j;

  constexpr Index2D()
    : Index2D(0, 0) {}
  constexpr Index2D(intType _i, intType _j)
    : i(_i), j(_j) {}

  constexpr intType& operator[](int index)
  {
    EN_ASSERT(0 <= index && index < 2, "Index is out of bounds!");
    return *(&i + index);
  }

  constexpr const intType& operator[](int index) const
  {
    EN_ASSERT(0 <= index && index < 2, "Index is out of bounds!");
    return *(&i + index);
  }

  // Comparison operators define lexicographical ordering on 3D indices
  constexpr bool operator==(const Index2D<intType>& other) const { return i == other.i && j == other.j; }
  constexpr bool operator!=(const Index2D<intType>& other) const { return !(*this == other); }
  constexpr bool operator<(const Index2D<intType>& other) const { return i < other.i || (i == other.i && j < other.j); }
  constexpr bool operator>(const Index2D<intType>& other) const { return i > other.i || (i == other.i && j > other.j); }
  constexpr bool operator<=(const Index2D<intType>& other) const { return !(*this > other); }
  constexpr bool operator>=(const Index2D<intType>& other) const { return !(*this < other); }

  constexpr Index2D<intType> operator-() const { return Index2D<intType>(-i, -j); };
  constexpr Index2D<intType> operator+=(const Index2D<intType>& other)
  {
    i += other.i;
    j += other.j;
    return *this;
  }
  constexpr Index2D<intType> operator-=(const Index2D<intType>& other)
  {
    i -= other.i;
    j -= other.j;
    return *this;
  }
  constexpr Index2D<intType> operator*=(intType n)
  {
    EN_ASSERT(n == 0 || (static_cast<intType>(i * n) / n == i && static_cast<intType>(j * n) / n == j), "Integer overflow!");
    i *= n;
    j *= n;
    return *this;
  }
  constexpr Index2D<intType> operator/=(intType n)
  {
    i /= n;
    j /= n;
    return *this;
  }

  constexpr Index2D<intType> operator+(Index2D<intType> other) const { return other += *this; }
  constexpr Index2D<intType> operator-(Index2D<intType> other) const { return -(other -= *this); }

  constexpr Index2D<intType> operator+(intType n) const { return Index2D<intType>(n, n) += *this; }
  constexpr Index2D<intType> operator-(intType n) const { return -(Index2D<intType>(n, n) -= *this); }

  constexpr Index2D<intType> operator*(intType n) const
  {
    Index2D<intType> copy = *this;
    return copy *= n;
  }
  constexpr Index2D<intType> operator/(intType n) const
  {
    Index2D<intType> copy = *this;
    return copy /= n;
  }

  explicit constexpr operator Vec2() const { return { i, j }; }

  static constexpr Index2D<intType> ToIndex(const Vec2& vec)
  {
    return { static_cast<intType>(vec.x), static_cast<intType>(vec.y) };
  }
};

/*
  Structs for indexing of blocks and chunks.

  BlockIndex indexes blocks within a chunk
  LocalIndex indexes chunks relative to the origin chunk (usually the chunk the player is in)
  GlobalIndex uniquely identifies a chunk within worldspace
*/
template<typename intType>
struct Index3D
{
  intType i;
  intType j;
  intType k;

  constexpr Index3D()
    : Index3D(0, 0, 0) {}
  constexpr Index3D(intType _i, intType _j, intType _k)
    : i(_i), j(_j), k(_k) {}
  constexpr Index3D(Index2D<intType> index2D, intType _k)
    : i(index2D.i), j(index2D.j), k(_k) {}

  constexpr intType& operator[](int index)
  {
    EN_ASSERT(0 <= index && index < 3, "Index is out of bounds!");
    return *(&i + index);
  }

  constexpr const intType& operator[](int index) const
  {
    EN_ASSERT(0 <= index && index < 3, "Index is out of bounds!");
    return *(&i + index);
  }

  // Comparison operators define lexicographical ordering on 3D indices
  constexpr bool operator==(const Index3D<intType>& other) const { return i == other.i && j == other.j && k == other.k; }
  constexpr bool operator!=(const Index3D<intType>& other) const { return !(*this == other); }
  constexpr bool operator<(const Index3D<intType>& other) const { return i < other.i || (i == other.i && j < other.j) || (i == other.i && j == other.j && k < other.k); }
  constexpr bool operator>(const Index3D<intType>& other) const { return i > other.i || (i == other.i && j > other.j) || (i == other.i && j == other.j && k > other.k); }
  constexpr bool operator<=(const Index3D<intType>& other) const { return !(*this > other); }
  constexpr bool operator>=(const Index3D<intType>& other) const { return !(*this < other); }

  constexpr Index3D<intType> operator-() const { return Index3D<intType>(-i, -j, -k); };
  constexpr Index3D<intType> operator+=(const Index3D<intType>& other)
  {
    i += other.i;
    j += other.j;
    k += other.k;
    return *this;
  }
  constexpr Index3D<intType> operator-=(const Index3D<intType>& other)
  {
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return *this;
  }
  constexpr Index3D<intType> operator*=(intType n)
  {
    EN_ASSERT(n == 0 || (static_cast<intType>(i * n) / n == i && static_cast<intType>(j * n) / n == j && static_cast<intType>(k * n) / n == k), "Integer overflow!");
    i *= n;
    j *= n;
    k *= n;
    return *this;
  }
  constexpr Index3D<intType> operator/=(intType n)
  {
    i /= n;
    j /= n;
    k /= n;
    return *this;
  }

  constexpr Index3D<intType> operator+(Index3D<intType> other) const { return other += *this; }
  constexpr Index3D<intType> operator-(Index3D<intType> other) const { return -(other -= *this); }

  constexpr Index3D<intType> operator+(intType n) const { return Index3D<intType>(n, n, n) += *this; }
  constexpr Index3D<intType> operator-(intType n) const { return -(Index3D<intType>(n, n, n) -= *this); }

  constexpr Index3D<intType> operator*(intType n) const
  {
    Index3D<intType> copy = *this;
    return copy *= n;
  }
  constexpr Index3D<intType> operator/(intType n) const
  {
    Index3D<intType> copy = *this;
    return copy /= n;
  }

  explicit constexpr operator Vec2() const { return { i, j }; }
  explicit constexpr operator Vec3() const { return { i, j, k }; }
  explicit constexpr operator Index2D<intType>() const { return { i, j }; }

  static constexpr Index3D<intType> ToIndex(const Vec3& vec)
  {
    return { static_cast<intType>(vec.x), static_cast<intType>(vec.y), static_cast<intType>(vec.z) };
  }

  static constexpr Index3D<intType> Dir(Block::Face face)
  {
    static constexpr Index3D<intType> directions[6] = { { -1, 0, 0}, { 1, 0, 0}, { 0, -1, 0}, { 0, 1, 0}, { 0, 0, -1}, { 0, 0, 1} };
                                                   //       West        East        South        North       Bottom        Top

    return directions[static_cast<int>(face)];
  }

  static constexpr Index3D<intType> CreatePermuted(intType i, intType j, intType k, int permutation)
  {
    EN_ASSERT(0 <= permutation && permutation < 3, "Not a valid permuation!");
    switch (permutation)
    {
      default:
      case 0: return Index3D<intType>(i, j, k);
      case 1: return Index3D<intType>(k, i, j);
      case 2: return Index3D<intType>(j, k, i);
    }
  }
};

template<typename intType>
constexpr Index3D<intType> operator*(intType n, Index3D<intType> index)
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
  template<typename intType>
  inline ostream& operator<<(ostream& os, const Index3D<intType>& index)
  {
    return os << '[' << static_cast<int>(index.i) << ", " << static_cast<int>(index.j) << ", " << static_cast<int>(index.k) << ']';
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