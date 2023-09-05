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

  template<std::integral NewIntType>
  explicit constexpr operator IBox3<NewIntType>() const { return IBox3<NewIntType>(static_cast<IVec3<NewIntType>>(min), static_cast<IVec3<NewIntType>>(max)); }

  // Define lexicographical ordering on stored IVec3s
  constexpr std::strong_ordering operator<=>(const IBox3<IntType>& other) const = default;

  constexpr IBox3 operator+(const IVec3<IntType>& intVec3) const
  {
    return IBox3(min + intVec3, max + intVec3);
  }

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

  constexpr IVec3<IntType> extents() const
  {
    EN_CORE_ASSERT(valid(), "Box is not valid!");
    return IVec3<IntType>(max.i - min.i, max.j - min.j, max.k - min.k);
  }

  constexpr int volume() const
  {
    if (!valid())
      return 0;

    IVec3<IntType> boxExtents = extents();
    return boxExtents.i * boxExtents.j * boxExtents.k;
  }

  constexpr IBox3<IntType>& expand(IntType n = 1)
  {
    for (Axis axis : Axes())
      expandAlongAxis(axis, n);
    return *this;
  }
  constexpr IBox3<IntType>& expandAlongAxis(Axis axis, IntType n = 1)
  {
    min[axis] -= n;
    max[axis] += n;
    return *this;
  }

  constexpr IBox3<IntType>& shrink(IntType n = 1)
  {
    for (Axis axis : Axes())
      shrinkAlongAxis(axis, n);
    return *this;
  }
  constexpr IBox3<IntType>& shrinkAlongAxis(Axis axis, IntType n = 1)
  {
    min[axis] += n;
    max[axis] -= n;
    return *this;
  }

  constexpr IntType limitAlongDirection(Direction direction) const
  {
    Axis axis = AxisOf(direction);
    return IsUpstream(direction) ? max[axis] - 1 : min[axis];
  }

  constexpr IBox3<IntType> face(Direction direction) const
  {
    Axis axis = AxisOf(direction);
    IntType faceNormalLimit = limitAlongDirection(direction);

    IVec3<IntType> faceLower = min;
    faceLower[axis] = faceNormalLimit;

    IVec3<IntType> faceUpper = max;
    faceUpper[axis] = faceNormalLimit + 1;

    return { faceLower, faceUpper };
  }
  constexpr IBox3<IntType> faceInterior(Direction direction) const
  {
    Axis u = AxisOf(direction);
    Axis v = Cycle(u);
    Axis w = Cycle(v);

    return face(direction).shrinkAlongAxis(v).shrinkAlongAxis(w);
  }

  constexpr IBox3<IntType> edge(Direction sideA, Direction sideB) const
  {
    EN_CORE_ASSERT(sideA != !sideB, "Opposite faces cannot form edge!");

    IBox3<IntType> faceA = face(sideA);
    IBox3<IntType> faceB = face(sideB);
    return Intersection(faceA, faceB);
  }
  constexpr IBox3<IntType> edgeInterior(Direction sideA, Direction sideB) const
  {
    Axis edgeAxis = GetMissing(AxisOf(sideA), AxisOf(sideB));
    return edge(sideA, sideB).shrinkAlongAxis(edgeAxis);
  }

  template<std::signed_integral IndexType>
  constexpr IBox3<IntType> corner(const IVec3<IndexType>& offset) const
  {
    EN_CORE_ASSERT(Engine::Debug::EqualsOneOf(offset.i, -1, 1)
                && Engine::Debug::EqualsOneOf(offset.j, -1, 1)
                && Engine::Debug::EqualsOneOf(offset.k, -1, 1), "Offset IVec3 must contains values of -1 or 1!");

    IVec3<IntType> cornerIndex(offset.i > 0 ? max.i - 1 : min.i, offset.j > 0 ? max.j - 1 : min.j, offset.k > 0 ? max.k - 1 : min.k);
    return { cornerIndex, cornerIndex + 1 };
  }

  template<InvocableWithReturnType<bool, const IVec3<IntType>&> F>
  bool allOf(const F& condition) const
  {
    EN_CORE_ASSERT(valid(), "Box is not valid!");

    IVec3<IntType> index;
    for (index.i = min.i; index.i < max.i; ++index.i)
      for (index.j = min.j; index.j < max.j; ++index.j)
        for (index.k = min.k; index.k < max.k; ++index.k)
          if (!condition(index))
            return false;
    return true;
  }

  template<InvocableWithReturnType<bool, const IVec3<IntType>&> F>
  bool noneOf(const F& condition) const
  {
    EN_CORE_ASSERT(valid(), "Box is not valid!");

    IVec3<IntType> index;
    for (index.i = min.i; index.i < max.i; ++index.i)
      for (index.j = min.j; index.j < max.j; ++index.j)
        for (index.k = min.k; index.k < max.k; ++index.k)
          if (condition(index))
            return false;
    return true;
  }

  template<InvocableWithReturnType<void, const IVec3<IntType>&> F>
  void forEach(const F& function) const
  {
    EN_CORE_ASSERT(valid(), "Box is not valid!");

    IVec3<IntType> index;
    for (index.i = min.i; index.i < max.i; ++index.i)
      for (index.j = min.j; index.j < max.j; ++index.j)
        for (index.k = min.k; index.k < max.k; ++index.k)
          function(index);  
  }

  static constexpr IBox3<IntType> Union(const IBox3<IntType>& boxA, const IBox3<IntType>& boxB)
  {
    return { ComponentWiseMin(boxA.min, boxB.min), ComponentWiseMax(boxA.max, boxB.max) };
  }

  static constexpr IBox3<IntType> Intersection(const IBox3<IntType>& boxA, const IBox3<IntType>& boxB)
  {
    return { ComponentWiseMax(boxA.min, boxB.min), ComponentWiseMin(boxA.max, boxB.max) };
  }

  static constexpr IBox3<IntType> VoidBox()
  {
    return { std::numeric_limits<IntType>::max(), std::numeric_limits<IntType>::min() };
  }
};



namespace std
{
  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const IBox3<IntType>& index)
  {
    return os << '(' << index.min << ", " << index.max << ')';
  }
}