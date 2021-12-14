#pragma once

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

  intType& operator[](uint8_t index)
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

  const intType& operator[](uint8_t index) const
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

  constexpr operator Vec3() const { return { i, j, k }; }
};

template<typename intType>
constexpr Index3D<intType> operator*(intType n, const Index3D<intType>& index)
{
  return { static_cast<intType>(n * index.i),
           static_cast<intType>(n * index.j),
           static_cast<intType>(n * index.k) };
}

using blockIndex_t = uint8_t;
using localIndex_t = int16_t;
using globalIndex_t = int64_t;

using BlockIndex = Index3D<blockIndex_t>;
using LocalIndex = Index3D<localIndex_t>;
using GlobalIndex = Index3D<globalIndex_t>;