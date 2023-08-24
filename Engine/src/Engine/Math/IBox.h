#pragma once
#include "IVec.h"

template<std::integral IntType>
struct IBox3
{
  IVec3<IntType> min;
  IVec3<IntType> max;

  constexpr IBox3()
    : min(), max() {}
  constexpr IBox3(const IVec3<IntType>& minCorner, const IVec3<IntType>& maxCorner)
    : min(minCorner), max(maxCorner) {}
  constexpr IBox3(IntType minCorner, IntType maxCorner)
    : min(minCorner), max(maxCorner) {}
  constexpr IBox3(IntType iMin, IntType jMin, IntType kMin, IntType iMax, IntType jMax, IntType kMax)
    : min(iMin, jMin, kMin), max(iMax, jMax, kMax) {}

  constexpr bool valid() const
  {
    for (int i = 0; i < 3; ++i)
      if (min.i > max.i)
        return false;
    return true;
  }

  constexpr bool encloses(const IVec3<IntType>& intVec3) const
  {
    for (int i = 0; i < 3; ++i)
      if (intVec3[i] < min[i] || intVec3[i] >= max[i])
        return false;
    return true;
  }

  constexpr int length(int dimension) const
  {
    EN_CORE_ASSERT(valid(), "Box is not valid!");
    return max[dimension] - min[dimension];
  }

  constexpr IVec3<IntType> extents() const
  {
    EN_CORE_ASSERT(valid(), "Box is not valid!");
    return { max.i - min.i, max.j - min.j, max.k - min.k };
  }

  constexpr int volume() const
  {
    EN_CORE_ASSERT(valid());
    IVec3<IntType> boxExtents = extents();
    return boxExtents.i * boxExtents.j * boxExtents.k;
  }

  template<typename F, typename... Args>
  bool AllOf(F condition, Args&&... args) const
  {
    EN_CORE_ASSERT(valid());

    IVec3<IntType> index;
    for (index.i = min.i; index.i < max.i; ++index.i)
      for (index.j = min.j; index.j < max.j; ++index.j)
        for (index.k = min.k; index.k < max.k; ++index.k)
          if (!condition(index, std::forward<Args>(args)...))
            return false;
    return true;
  }

  template<typename F, typename... Args>
  void forEach(F function, Args&&... args) const
  {
    EN_CORE_ASSERT(valid());

    IVec3<IntType> index;
    for (index.i = min.i; index.i < max.i; ++index.i)
      for (index.j = min.j; index.j < max.j; ++index.j)
        for (index.k = min.k; index.k < max.k; ++index.k)
          function(index, std::forward<Args>(args)...);
  }

  static constexpr IBox3<IntType> Union(const IBox3<IntType>& other)
  {
    return { componentWiseMin(min, other.min), componentWiseMax(max, other.max) };
  }

  static constexpr IBox3<IntType> VoidBox()
  {
    return { std::numeric_limits<IntType>::max(), std::numeric_limits<IntType>::min() };
  }
};