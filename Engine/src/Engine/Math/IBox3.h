#pragma once
#include "IVec3.h"
#include "Engine/Core/Concepts.h" 

namespace eng::math
{
  /*
    Represents a box on a 3D integer lattice. Min and max bounds are both inclusive.
  */
  template<std::integral IntType>
  struct IBox3
  {
    IVec3<IntType> min;
    IVec3<IntType> max;
  
    constexpr IBox3()
      : min(), max() {}
    constexpr IBox3(IntType minCorner, IntType maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox3(const IVec3<IntType>& minCorner, const IVec3<IntType>& maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox3(IntType iMin, IntType jMin, IntType kMin, IntType iMax, IntType jMax, IntType kMax)
      : min(iMin, jMin, kMin), max(iMax, jMax, kMax) {}
  
    template<std::integral NewIntType>
    explicit constexpr operator IBox3<NewIntType>() const { return IBox3<NewIntType>(static_cast<IVec3<NewIntType>>(min), static_cast<IVec3<NewIntType>>(max)); }
  
    // Define lexicographical ordering on stored IVec3s
    constexpr std::strong_ordering operator<=>(const IBox3& other) const = default;
  
    constexpr IBox3& operator+=(const IVec3<IntType>& iVec3)
    {
      min += iVec3;
      max += iVec3;
      return *this;
    }
    constexpr IBox3& operator-=(const IVec3<IntType>& iVec3) { return *this += -iVec3; }
    constexpr IBox3& operator+=(IntType n) { return *this += IBox3(n, n); }
    constexpr IBox3& operator-=(IntType n) { return *this -= IBox3(n, n); }
  
    constexpr IBox3& operator*=(IntType n)
    {
      min *= n;
      max *= n;
      return *this;
    }
    constexpr IBox3& operator/=(IntType n)
    {
      min /= n;
      max /= n;
      return *this;
    }
  
    constexpr IBox3 operator-() const { return IBox3(-min, -max); }
  
    constexpr IBox3 operator+(const IVec3<IntType>& iVec3) const { return clone(*this) += iVec3; }
    constexpr IBox3 operator-(const IVec3<IntType>& iVec3) const { return clone(*this) -= iVec3; }
  
    constexpr IBox3 operator+(IntType n) const { return clone(*this) += n; }
    constexpr IBox3 operator-(IntType n) const { return clone(*this) -= n; }
  
    constexpr IBox3 operator*(IntType n) const { return clone(*this) *= n; }
    constexpr IBox3 operator/(IntType n) const { return clone(*this) /= n; }
  
    /*
      \returns True if the box dimensions are non-negative.
    */
    constexpr bool valid() const { return min.i <= max.i && min.j <= max.j && min.k <= max.k; }
  
    /*
      \returns True if the given point is contained within the box.
    */
    constexpr bool encloses(const IVec3<IntType>& iVec3) const
    {
      for (Axis axis : Axes())
        if (iVec3[axis] < min[axis] || iVec3[axis] > max[axis])
          return false;
      return true;
    }
  
    /*
      \returns The box dimensions.
    */
    constexpr IVec3<IntType> extents() const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
      return IVec3<IntType>(max.i - min.i + 1, max.j - min.j + 1, max.k - min.k + 1);
    }
  
    /*
      \returns The number of integers contained within the box.
    */
    constexpr size_t volume() const
    {
      if (!valid())
        return 0;
  
      IVec3<size_t> boxExtents = static_cast<IVec3<size_t>>(extents());
      return boxExtents.i * boxExtents.j * boxExtents.k;
    }
  
    constexpr int linearIndexOf(const IVec3<IntType>& index) const
    {
      ENG_CORE_ASSERT(encloses(index), "Index is outside box!");
      IVec3<IntType> boxExtents = extents();
      IVec3<IntType> strides(boxExtents.j * boxExtents.k, boxExtents.k, 1);
      IVec3<IntType> indexRelativeToBase = index - min;
      return strides.dot(indexRelativeToBase);
    }
  
    constexpr IBox3& expand(IntType n = 1)
    {
      for (Axis axis : Axes())
        expandAlongAxis(axis, n);
      return *this;
    }
    constexpr IBox3& expandAlongAxis(Axis axis, IntType n = 1)
    {
      min[axis] -= n;
      max[axis] += n;
      return *this;
    }
    constexpr IBox3& expandToEnclose(const IVec3<IntType>& iVec3)
    {
      min = ComponentWiseMin(min, iVec3);
      max = ComponentWiseMax(max, iVec3 + 1);
      return *this;
    }
    constexpr IBox3& shrink(IntType n = 1) { return expand(-n); }
    constexpr IBox3& shrinkAlongAxis(Axis axis, IntType n = 1) { return expandAlongAxis(axis, -n); }
  
    constexpr IntType limitAlongDirection(Direction direction) const
    {
      const IVec3<IntType>& limit = IsUpstream(direction) ? max : min;
      return limit[AxisOf(direction)];
    }
  
    constexpr IBox3 face(Direction side) const
    {
      Axis axis = AxisOf(side);
      IntType faceNormalLimit = limitAlongDirection(side);
  
      IVec3<IntType> faceLower = min;
      faceLower[axis] = faceNormalLimit;
  
      IVec3<IntType> faceUpper = max;
      faceUpper[axis] = faceNormalLimit;
  
      return IBox3(faceLower, faceUpper);
    }
    constexpr IBox3 faceInterior(Direction side) const
    {
      Axis u = AxisOf(side);
      Axis v = Cycle(u);
      Axis w = Cycle(v);
  
      return face(side).shrinkAlongAxis(v).shrinkAlongAxis(w);
    }
  
    constexpr IBox3 edge(Direction sideA, Direction sideB) const
    {
      ENG_CORE_ASSERT(sideA != !sideB, "Opposite faces cannot form edge!");
  
      IBox3 faceA = face(sideA);
      IBox3 faceB = face(sideB);
      return Intersection(faceA, faceB);
    }
    constexpr IBox3 edgeInterior(Direction sideA, Direction sideB) const
    {
      Axis edgeAxis = GetMissing(AxisOf(sideA), AxisOf(sideB));
      return edge(sideA, sideB).shrinkAlongAxis(edgeAxis);
    }
  
    template<std::integral IndexType>
    constexpr IBox3 corner(const IVec3<IndexType>& offset) const
    {
      ENG_CORE_ASSERT(debug::EqualsOneOf(offset.i, -1, 1)
                  && debug::EqualsOneOf(offset.j, -1, 1)
                  && debug::EqualsOneOf(offset.k, -1, 1), "Offset IVec3 must contains values of -1 or 1!");
  
      IVec3<IntType> cornerIndex(offset.i > 0 ? max.i : min.i, offset.j > 0 ? max.j : min.j, offset.k > 0 ? max.k : min.k);
      return { cornerIndex, cornerIndex };
    }
  
    template<InvocableWithReturnType<bool, const IVec3<IntType>&> F>
    bool allOf(const F& condition) const
    {
      return noneOf([&condition](const IVec3<IntType>& index) { return !condition(index); });
    }
  
    template<InvocableWithReturnType<bool, const IVec3<IntType>&> F>
    bool anyOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec3<IntType> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          for (index.k = min.k; index.k <= max.k; ++index.k)
            if (condition(index))
              return true;
      return false;
    }
  
    template<InvocableWithReturnType<bool, const IVec3<IntType>&> F>
    bool noneOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec3<IntType> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          for (index.k = min.k; index.k <= max.k; ++index.k)
            if (condition(index))
              return false;
      return true;
    }
  
    template<InvocableWithReturnType<void, const IVec3<IntType>&> F>
    void forEach(const F& function) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec3<IntType> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          for (index.k = min.k; index.k <= max.k; ++index.k)
            function(index);  
    }
  
    static constexpr IBox3 Intersection(const IBox3& boxA, const IBox3& boxB)
    {
      return { ComponentWiseMax(boxA.min, boxB.min), ComponentWiseMin(boxA.max, boxB.max) };
    }
  
    static constexpr IBox3 VoidBox()
    {
      return { std::numeric_limits<IntType>::max(), std::numeric_limits<IntType>::min() };
    }
  };
  
  
  
  template<std::integral IntType>
  struct BoxFace
  {
    Direction side;
    IBox3<IntType> bounds;
  
    constexpr BoxFace()
      : BoxFace(Direction::First, {}) {}
    constexpr BoxFace(Direction faceSide, const IBox3<IntType>& faceBounds)
      : side(faceSide), bounds(faceBounds) {}
  };
  
  template<std::integral IntType>
  struct BoxEdge
  {
    Direction sideA;
    Direction sideB;
    IBox3<IntType> bounds;
  
    constexpr BoxEdge()
      : BoxEdge(Direction::First, Direction::First, {}) {}
    constexpr BoxEdge(Direction edgeSideA, Direction edgeSideB, const IBox3<IntType>& edgeBounds)
      : sideA(edgeSideA), sideB(edgeSideB), bounds(edgeBounds) {}
  };
  
  template<std::integral IntType>
  struct BoxCorner
  {
    IVec3<IntType> offset;
    IBox3<IntType> bounds;
  
    constexpr BoxCorner() = default;
    constexpr BoxCorner(const IVec3<IntType>& cornerOffset, const IBox3<IntType>& cornerBounds)
      : offset(cornerOffset), bounds(cornerBounds) {}
  };
  
  namespace detail
  {
    template<std::integral IntType>
    constexpr std::array<BoxFace<IntType>, 6> ConstructFaces(const IBox3<IntType>& box, bool interiorOnly)
    {
      std::array<BoxFace<IntType>, 6> faces;
      for (Direction side : Directions())
        faces[static_cast<int>(side)] = BoxFace(side, interiorOnly ? box.faceInterior(side) : box.face(side));
      return faces;
    }
  
    template<std::integral IntType>
    constexpr std::array<BoxEdge<IntType>, 12> ConstructEdges(const IBox3<IntType>& box, bool interiorOnly)
    {
      int edgeIndex = 0;
      std::array<BoxEdge<IntType>, 12> edges;
      for (auto itA = Directions().begin(); itA != Directions().end(); ++itA)
        for (auto itB = itA.next(); itB != Directions().end(); ++itB)
        {
          Direction sideA = *itA;
          Direction sideB = *itB;
  
          // Opposite faces cannot form edge
          if (sideA == !sideB)
            continue;
  
          edges[edgeIndex] = BoxEdge(sideA, sideB, interiorOnly ? box.edgeInterior(sideA, sideB) : box.edge(sideA, sideB));
          edgeIndex++;
        }
      return edges;
    }
  
    template<std::integral IntType>
    constexpr std::array<BoxCorner<IntType>, 8> ConstructCorners(const IBox3<IntType>& box)
    {
      int cornerIndex = 0;
      std::array<BoxCorner<IntType>, 8> corners;
      for (int i = -1; i < 2; i += 2)
        for (int j = -1; j < 2; j += 2)
          for (int k = -1; k < 2; k += 2)
          {
            IVec3<IntType> offset(i, j, k);
            corners[cornerIndex] = BoxCorner(offset, box.corner(offset));
            cornerIndex++;
          }
      return corners;
    }
  }
  
  template<std::integral IntType>
  constexpr std::array<BoxFace<IntType>, 6> Faces(const IBox3<IntType>& box)
  {
    return detail::ConstructFaces(box, false);
  }
  
  template<std::integral IntType>
  constexpr std::array<BoxFace<IntType>, 6> FaceInteriors(const IBox3<IntType>& box)
  {
    return detail::ConstructFaces(box, true);
  }
  
  template<std::integral IntType>
  constexpr std::array<BoxEdge<IntType>, 12> Edges(const IBox3<IntType>& box)
  {
    return detail::ConstructEdges(box, false);
  }
  
  template<std::integral IntType>
  constexpr std::array<BoxEdge<IntType>, 12> EdgeInteriors(const IBox3<IntType>& box)
  {
    return detail::ConstructEdges(box, true);
  }
  
  template<std::integral IntType>
  constexpr std::array<BoxCorner<IntType>, 8> Corners(const IBox3<IntType>& box)
  {
    return detail::ConstructCorners(box);
  }
}



namespace std
{
  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const eng::math::IBox3<IntType>& box)
  {
    return os << '(' << box.min << ", " << box.max << ')';
  }
}