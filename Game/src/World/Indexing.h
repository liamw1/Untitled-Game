#pragma once
#include "Block/Block.h"
#include <llvm/ADT/DenseMapInfo.h>

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

  constexpr Index3D() = default;
  constexpr Index3D(intType i, intType j, intType k)
    : i(i), j(j), k(k) {}

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

  constexpr operator Vec2() const { return { i, j }; }
  constexpr operator Vec3() const { return { i, j, k }; }

  static constexpr Index3D<intType> ToIndex(const Vec3& vec)
  {
    return { static_cast<intType>(vec.x),
             static_cast<intType>(vec.y),
             static_cast<intType>(vec.z) };
  }

  static constexpr Index3D<intType> OutwardNormal(Block::Face face)
  {
    static constexpr Index3D<intType> normals[6] = { { -1, 0, 0}, { 1, 0, 0}, { 0, -1, 0}, { 0, 1, 0}, { 0, 0, -1}, { 0, 0, 1} };
                                                //       West        East        South        North       Bottom        Top

    return normals[static_cast<int>(face)];
  }
};

template<typename intType>
constexpr Index3D<intType> operator+(Index3D<intType> left, const Index3D<intType>& right)
{
  left += right;
  return left;
}
template<typename intType>
constexpr Index3D<intType> operator-(Index3D<intType> left, const Index3D<intType>& right)
{
  left -= right;
  return left;
}

template<typename intType>
constexpr Index3D<intType> operator+(Index3D<intType> index, intType n)
{
  index += n * Index3D<intType>(1, 1, 1);
  return index;
}
template<typename intType>
constexpr Index3D<intType> operator-(Index3D<intType> index, intType n)
{
  index -= n * Index3D<intType>(1, 1, 1);
  return index;
}

template<typename intType>
constexpr Index3D<intType> operator*(intType n, const Index3D<intType>& index)
{
  return { static_cast<intType>(n * index.i),
           static_cast<intType>(n * index.j),
           static_cast<intType>(n * index.k) };
}

namespace std
{
  template<typename intType>
  inline ostream& operator<<(ostream& os, const Index3D<intType>& index)
  {
    return os << '(' << static_cast<int>(index.i) << ", " << static_cast<int>(index.j) << ", " << static_cast<int>(index.k) << ')';
  }
}



// =========== Precision selection for Indices ============= //

template <bool isDoublePrecision> struct GlobalIndexSelector;

template<> struct GlobalIndexSelector<true> { using type = typename int64_t; };
template<> struct GlobalIndexSelector<false> { using type = typename int32_t; };

using blockIndex_t = int8_t;
using localIndex_t = int16_t;
using globalIndex_t = typename GlobalIndexSelector<std::is_same<double, length_t>::value>::type;

using BlockIndex = Index3D<blockIndex_t>;
using LocalIndex = Index3D<localIndex_t>;
using GlobalIndex = Index3D<globalIndex_t>;