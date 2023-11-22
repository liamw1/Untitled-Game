#pragma once
#include "IVec3.h"
#include "Engine/Core/Concepts.h" 

namespace eng::math
{
  /*
    Represents a box on a 3D integer lattice. Min and max bounds are both inclusive.

    TODO: Create iterators that loop over integers contained in box.
  */
  template<std::integral T>
  class IBox3
  {
  public:
    IVec3<T> min;
    IVec3<T> max;
  
    constexpr IBox3()
      : min(), max() {}
    constexpr IBox3(T minCorner, T maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox3(const IVec3<T>& minCorner, const IVec3<T>& maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox3(T iMin, T jMin, T kMin, T iMax, T jMax, T kMax)
      : min(iMin, jMin, kMin), max(iMax, jMax, kMax) {}

    template<std::integral U>
    constexpr IBox3<U> upcast() const { return { min.upcast<U>(), max.upcast<U>() }; }

    template<std::integral U>
    constexpr IBox3<U> checkedCast() const { return { min.checkedCast<U>(), max.checkedCast<U>() }; }

    template<std::integral U>
    constexpr IBox3<U> uncheckedCast() const { return { min.uncheckedCast<U>(), max.uncheckedCast<U>() }; }
  
    // Define lexicographical ordering on stored IVec3s
    constexpr std::strong_ordering operator<=>(const IBox3& other) const = default;
  
    constexpr IBox3& operator+=(const IVec3<T>& iVec3)
    {
      min += iVec3;
      max += iVec3;
      return *this;
    }
    constexpr IBox3& operator-=(const IVec3<T>& iVec3) { return *this += -iVec3; }
    constexpr IBox3& operator+=(T n) { return *this += IBox3(n, n); }
    constexpr IBox3& operator-=(T n) { return *this -= IBox3(n, n); }
  
    constexpr IBox3& operator*=(T n)
    {
      min *= n;
      max *= n;
      return *this;
    }
    constexpr IBox3& operator/=(T n)
    {
      min /= n;
      max /= n;
      return *this;
    }
  
    constexpr IBox3 operator-() const { return IBox3(-min, -max); }
  
    constexpr IBox3 operator+(const IVec3<T>& iVec3) const { return clone(*this) += iVec3; }
    constexpr IBox3 operator-(const IVec3<T>& iVec3) const { return clone(*this) -= iVec3; }
  
    constexpr IBox3 operator+(T n) const { return clone(*this) += n; }
    constexpr IBox3 operator-(T n) const { return clone(*this) -= n; }
  
    constexpr IBox3 operator*(T n) const { return clone(*this) *= n; }
    constexpr IBox3 operator/(T n) const { return clone(*this) /= n; }
  
    /*
      \returns True if the box dimensions are non-negative.
    */
    constexpr bool valid() const { return min.i <= max.i && min.j <= max.j && min.k <= max.k; }
  
    /*
      \returns True if the given point is contained within the box.
    */
    constexpr bool encloses(const IVec3<T>& iVec3) const
    {
      return algo::noneOf(Axes(), [this, &iVec3](Axis axis) { return iVec3[axis] < min[axis] || iVec3[axis] > max[axis]; });
    }
  
    /*
      \returns The box dimensions.
    */
    constexpr IVec3<std::make_unsigned_t<T>> extents() const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
      return IVec3<std::make_unsigned_t<T>>(max.i - min.i + 1, max.j - min.j + 1, max.k - min.k + 1);
    }
  
    /*
      \returns The number of integers contained within the box.
    */
    constexpr uSize volume() const
    {
      if (!valid())
        return 0;
  
      IVec3<uSize> boxExtents = extents().upcast<uSize>();
      return boxExtents.i * boxExtents.j * boxExtents.k;
    }

    /*
      \returns The 1D index for a point inside of the box, with the 0th index
               at min and the final index at max.
    */
    constexpr uSize linearIndexOf(const IVec3<T>& index) const
    {
      IVec3<uSize> boxExtents = extents().upcast<uSize>();
      IVec3<uSize> strides(boxExtents.j * boxExtents.k, boxExtents.k, 1);
      IVec3<uSize> indexRelativeToBase = (index - min).checkedCast<uSize>();
      return strides.dot(indexRelativeToBase);
    }
  
    constexpr IBox3& expand(T n = 1)
    {
      for (Axis axis : Axes())
        expandAlongAxis(axis, n);
      return *this;
    }
    [[nodiscard]] constexpr IBox3 expand(T n = 1) const { return eng::clone(*this).expand(n); }

    constexpr IBox3& expandAlongAxis(Axis axis, T n = 1)
    {
      min[axis] -= n;
      max[axis] += n;
      return *this;
    }
    [[nodiscard]] constexpr IBox3 expandAlongAxis(Axis axis, T n = 1) const { return eng::clone(*this).expandAlongAxis(axis, n); }

    constexpr IBox3& expandToEnclose(const IVec3<T>& iVec3)
    {
      min = ComponentWiseMin(min, iVec3);
      max = ComponentWiseMax(max, iVec3);
      return *this;
    }
    [[nodiscard]] constexpr IBox3 expandToEnclose(const IVec3<T>& iVec3) const { return eng::clone(*this).expandToEnclose(iVec3); }

    constexpr IBox3& expandToEnclose(const IBox3& box)
    {
      expandToEnclose(box.min);
      expandToEnclose(box.max);
      return *this;
    }
    [[nodiscard]] constexpr IBox3 expandToEnclose(const IBox3& box) const { return eng::clone(*this).expandToEnclose(box); }

    constexpr IBox3& shrink(T n = 1) { return expand(-n); }
    [[nodiscard]] constexpr IBox3 shrink(T n = 1) const { return eng::clone(*this).shrink(n); }

    constexpr IBox3& shrinkAlongAxis(Axis axis, T n = 1) { return expandAlongAxis(axis, -n); }
    [[nodiscard]] constexpr IBox3 shrinkAlongAxis(Axis axis, T n = 1) const { return eng::clone(*this).shrinkAlongAxis(axis, n); }
  
    constexpr T limitAlongDirection(Direction direction) const
    {
      const IVec3<T>& limit = isUpstream(direction) ? max : min;
      return limit[axisOf(direction)];
    }
  
    constexpr IBox3 face(Direction side) const
    {
      Axis axis = axisOf(side);
      T faceNormalLimit = limitAlongDirection(side);
  
      IVec3<T> faceLower = min;
      faceLower[axis] = faceNormalLimit;
  
      IVec3<T> faceUpper = max;
      faceUpper[axis] = faceNormalLimit;
  
      return IBox3(faceLower, faceUpper);
    }
    constexpr IBox3 faceInterior(Direction side) const
    {
      Axis u = axisOf(side);
      Axis v = cycle(u);
      Axis w = cycle(v);
  
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
      Axis edgeAxis = getMissing(axisOf(sideA), axisOf(sideB));
      return edge(sideA, sideB).shrinkAlongAxis(edgeAxis);
    }
  
    template<std::integral IndexType>
    constexpr IBox3 corner(const IVec3<IndexType>& offset) const
    {
      ENG_CORE_ASSERT(debug::equalsOneOf(offset.i, -1, 1) &&
                      debug::equalsOneOf(offset.j, -1, 1) &&
                      debug::equalsOneOf(offset.k, -1, 1), "Offset IVec3 must contains values of -1 or 1!");
  
      IVec3<T> cornerIndex(offset.i > 0 ? max.i : min.i, offset.j > 0 ? max.j : min.j, offset.k > 0 ? max.k : min.k);
      return { cornerIndex, cornerIndex };
    }
  
    template<std::predicate<const IVec3<T>&> F>
    constexpr bool allOf(const F& condition) const
    {
      return noneOf([&condition](const IVec3<T>& index) { return !condition(index); });
    }
  
    template<std::predicate<const IVec3<T>&> F>
    constexpr bool anyOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec3<T> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          for (index.k = min.k; index.k <= max.k; ++index.k)
            if (condition(index))
              return true;
      return false;
    }
  
    template<std::predicate<const IVec3<T>&> F>
    constexpr bool noneOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec3<T> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          for (index.k = min.k; index.k <= max.k; ++index.k)
            if (condition(index))
              return false;
      return true;
    }
  
    template<InvocableWithReturnType<void, const IVec3<T>&> F>
    constexpr void forEach(const F& function) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec3<T> index;
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
      return { std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest() };
    }
  };
  
  
  
  namespace detail
  {
    template<std::integral T>
    constexpr std::array<IBox3<T>, 6> ConstructFaces(const IBox3<T>& box, bool interiorOnly)
    {
      std::array<IBox3<T>, 6> faces;
      for (Direction side : Directions())
        faces[toUnderlying(side)] = interiorOnly ? box.faceInterior(side) : box.face(side);
      return faces;
    }
  
    template<std::integral T>
    constexpr std::array<IBox3<T>, 12> ConstructEdges(const IBox3<T>& box, bool interiorOnly)
    {
      i32 edgeIndex = 0;
      std::array<IBox3<T>, 12> edges;
      for (auto itA = Directions().begin(); itA != Directions().end(); ++itA)
        for (auto itB = itA.next(); itB != Directions().end(); ++itB)
        {
          Direction sideA = *itA;
          Direction sideB = *itB;
  
          // Opposite faces cannot form edge
          if (sideA == !sideB)
            continue;
  
          edges[edgeIndex++] = interiorOnly ? box.edgeInterior(sideA, sideB) : box.edge(sideA, sideB);
        }
      return edges;
    }
  }
  
  template<std::integral T>
  constexpr std::array<IBox3<T>, 6> Faces(const IBox3<T>& box) { return detail::ConstructFaces(box, false); }

  template<std::integral T>
  constexpr std::array<IBox3<T>, 6> FaceInteriors(const IBox3<T>& box) { return detail::ConstructFaces(box, true); }
  
  template<std::integral T>
  constexpr std::array<IBox3<T>, 12> Edges(const IBox3<T>& box) { return detail::ConstructEdges(box, false); }
  
  template<std::integral T>
  constexpr std::array<IBox3<T>, 12> EdgeInteriors(const IBox3<T>& box) { return detail::ConstructEdges(box, true); }
  
  template<std::integral T>
  constexpr std::array<IBox3<T>, 8> Corners(const IBox3<T>& box)
  {
    i32 cornerIndex = 0;
    std::array<IBox3<T>, 8> corners;
    for (i32 i = -1; i < 2; i += 2)
      for (i32 j = -1; j < 2; j += 2)
        for (i32 k = -1; k < 2; k += 2)
          corners[cornerIndex++] = box.corner(IVec3<T>(i, j, k));
    return corners;
  }
}



namespace std
{
  template<std::integral T>
  inline ostream& operator<<(ostream& os, const eng::math::IBox3<T>& box)
  {
    return os << '(' << box.min << ", " << box.max << ')';
  }
}