#pragma once
#include "IVec2.h"
#include "Engine/Core/Concepts.h"

namespace eng::math
{
  /*
    Represents a box on a 2D integer lattice. Min and max bounds are both inclusive.

    TODO: Create iterators that loop over integers contained in box.
  */
  template<std::integral T>
  class IBox2
  {
  public:
    IVec2<T> min;
    IVec2<T> max;
  
    constexpr IBox2()
      : min(), max() {}
    constexpr IBox2(T minCorner, T maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox2(const IVec2<T>& minCorner, const IVec2<T>& maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox2(T iMin, T jMin, T iMax, T jMax)
      : min(iMin, jMin), max(iMax, jMax) {}

    template<std::integral U>
    constexpr IBox2<U> upcast() const { return { min.upcast<U>(), max.upcast<U>() }; }

    template<std::integral U>
    constexpr IBox2<U> checkedCast() const { return { min.checkedCast<U>(), max.checkedCast<U>() }; }
  
    // Define lexicographical ordering on stored IVec2s
    constexpr std::strong_ordering operator<=>(const IBox2& other) const = default;
  
    constexpr IBox2& operator+=(const IVec2<T>& iVec2)
    {
      min += iVec2;
      max += iVec2;
      return *this;
    }
    constexpr IBox2& operator-=(const IVec2<T>& iVec2) { return *this += -iVec2; }
    constexpr IBox2& operator+=(T n) { return *this += IBox2(n, n); }
    constexpr IBox2& operator-=(T n) { return *this -= IBox2(n, n); }
  
    constexpr IBox2& operator*=(T n)
    {
      min *= n;
      max *= n;
      return *this;
    }
    constexpr IBox2& operator/=(T n)
    {
      min /= n;
      max /= n;
      return *this;
    }
  
    constexpr IBox2 operator-() const { return IBox2(-min, -max); }
  
    constexpr IBox2 operator+(const IVec2<T>& iVec2) const { return Clone(*this) += iVec2; }
    constexpr IBox2 operator-(const IVec2<T>& iVec2) const { return Clone(*this) -= iVec2; }
  
    constexpr IBox2 operator+(T n) const { return Clone(*this) += n; }
    constexpr IBox2 operator-(T n) const { return Clone(*this) -= n; }
  
    constexpr IBox2 operator*(T n) const { return Clone(*this) *= n; }
    constexpr IBox2 operator/(T n) const { return Clone(*this) /= n; }
  
    /*
      \returns True if the box dimensions are non-negative.
    */
    constexpr bool valid() const { return min.i <= max.i && min.j <= max.j; }
  
    /*
      \returns True if the given point is contained within the box.
    */
    constexpr bool encloses(const IVec2<T>& iVec2) const
    {
      return iVec2.i >= min.i && iVec2.i <= max.i && iVec2.j >= min.j && iVec2.j <= max.j;
    }
  
    /*
      \returns The box dimensions.
    */
    constexpr IVec2<std::make_unsigned_t<T>> extents() const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
      return IVec2<std::make_unsigned_t<T>>(max.i - min.i + 1, max.j - min.j + 1);
    }
  
    /*
      \returns The number of integers contained within the box.
    */
    constexpr uSize volume() const
    {
      if (!valid())
        return 0;
  
      IVec2<uSize> boxExtents = extents().upcast<uSize>();
      return boxExtents.i * boxExtents.j;
    }
  
    constexpr uSize linearIndexOf(const IVec2<T>& index) const
    {
      IVec2<uSize> boxExtents = extents().upcast<uSize>();
      IVec2<uSize> strides(boxExtents.j, 1);
      IVec2<uSize> indexRelativeToBase = (index - min).checkedCast<uSize>();
      return strides.dot(indexRelativeToBase);
    }
  
    constexpr IBox2& expand(T n = 1)
    {
      min -= n;
      max += n;
      return *this;
    }
    constexpr IBox2& shrink(T n = 1) { return expand(-n); }
  
    template<std::predicate<const IVec2<T>&> F>
    bool allOf(const F& condition) const
    {
      return noneOf([&condition](const IVec2<T>& index) { return !condition(index); });
    }
  
    template<std::predicate<const IVec2<T>&> F>
    bool anyOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec2<T> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          if (condition(index))
            return true;
      return false;
    }
  
    template<std::predicate<const IVec2<T>&> F>
    bool noneOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec2<T> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          if (condition(index))
            return false;
      return true;
    }
  
    template<InvocableWithReturnType<void, const IVec2<T>&> F>
    void forEach(const F& function) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec2<T> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          function(index);  
    }
  
    static constexpr IBox2 Intersection(const IBox2& boxA, const IBox2& boxB)
    {
      return { ComponentWiseMax(boxA.min, boxB.min), ComponentWiseMin(boxA.max, boxB.max) };
    }
  
    static constexpr IBox2 VoidBox()
    {
      return { std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest() };
    }
  };
}



namespace std
{
  template<std::integral T>
  inline ostream& operator<<(ostream& os, const eng::math::IBox2<T>& box)
  {
    return os << '(' << box.min << ", " << box.max << ')';
  }
}
