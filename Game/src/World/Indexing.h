#pragma once
#include "Block/Block.h"

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

  intType& operator[](int index)
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

  const intType& operator[](int index) const
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

  constexpr bool operator==(const Index3D<intType>& other) const { return i == other.i && j == other.j && k == other.k; }

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

  static const Index3D<intType>& OutwardNormal(BlockFace face)  // NOTE: Can't declare this as contexpr, not sure why
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

using blockIndex_t = int8_t;
using localIndex_t = int16_t;
using globalIndex_t = int64_t;

using BlockIndex = Index3D<blockIndex_t>;
using LocalIndex = Index3D<localIndex_t>;
using GlobalIndex = Index3D<globalIndex_t>;