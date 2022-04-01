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
    EN_ASSERT(index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  constexpr const intType& operator[](int index) const
  {
    EN_ASSERT(index < 3, "Index is out of bounds!");
    switch (index)
    {
      default:
      case 0: return i;
      case 1: return j;
      case 2: return k;
    }
  }

  // Comparison operators define lexicographical ordering on 3D indices
  constexpr bool operator==(const Index3D<intType>& other) const { return i == other.i && j == other.j && k == other.k; }
  constexpr bool operator!=(const Index3D<intType>& other) const { return !(*this == other); }
  constexpr bool operator<(const Index3D<intType>& other) const { return i < other.i || (i == other.i && j < other.j) || (i == other.i && j == other.j && k < other.k); }
  constexpr bool operator>(const Index3D<intType>& other) const { return i > other.i || (i == other.i && j > other.j) || (i == other.i && j == other.j && k > other.k); }
  constexpr bool operator<=(const Index3D<intType>& other) const { return !(*this > other); }
  constexpr bool operator>=(const Index3D<intType>& other) const { return !(*this < other); }

  constexpr Index3D<intType> operator+(const Index3D<intType>& other) const
  {
    return { static_cast<intType>(i + other.i),
             static_cast<intType>(j + other.j),
             static_cast<intType>(k + other.k) };
  }
  constexpr Index3D<intType> operator-(const Index3D<intType>& other) const
  {
    return { static_cast<intType>(i - other.i),
             static_cast<intType>(j - other.j),
             static_cast<intType>(k - other.k) };
  }

  constexpr Index3D<intType> operator+(intType n) const
  {
    return { static_cast<intType>(i + n),
             static_cast<intType>(j + n),
             static_cast<intType>(k + n) };
  }
  constexpr Index3D<intType> operator-(intType n) const
  {
    return { static_cast<intType>(i - n),
             static_cast<intType>(j - n),
             static_cast<intType>(k - n) };
  }

  constexpr operator Vec2() const { return { i, j }; }
  constexpr operator Vec3() const { return { i, j, k }; }

  static constexpr Index3D<intType> ToIndex(const Vec3& vec)
  {
    return { static_cast<intType>(vec.x),
             static_cast<intType>(vec.y),
             static_cast<intType>(vec.z) };
  }

  static const Index3D<intType>& OutwardNormal(Block::Face face)  // NOTE: Can't declare this as contexpr, not sure why
  {
    static constexpr Index3D<intType> normals[6] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                                //      East         West        North       South         Top        Bottom

    return normals[static_cast<int>(face)];
  }
};

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
    return os << '(' << index.i << ", " << index.j << ", " << index.k << ')';
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