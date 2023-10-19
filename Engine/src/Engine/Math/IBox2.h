#pragma once
#include "IVec2.h"
#include "Engine/Core/Concepts.h"

namespace eng::math
{
  /*
    Represents a box on a 2D integer lattice. Min and max bounds are both inclusive.
  */
  template<std::integral IntType>
  struct IBox2
  {
    IVec2<IntType> min;
    IVec2<IntType> max;
  
    constexpr IBox2()
      : min(), max() {}
    constexpr IBox2(IntType minCorner, IntType maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox2(const IVec2<IntType>& minCorner, const IVec2<IntType>& maxCorner)
      : min(minCorner), max(maxCorner) {}
    constexpr IBox2(IntType iMin, IntType jMin, IntType iMax, IntType jMax)
      : min(iMin, jMin), max(iMax, jMax) {}
  
    template<std::integral NewIntType>
    explicit constexpr operator IBox2<NewIntType>() const { return IBox2<NewIntType>(static_cast<IVec2<NewIntType>>(min), static_cast<IVec2<NewIntType>>(max)); }
  
    // Define lexicographical ordering on stored IVec2s
    constexpr std::strong_ordering operator<=>(const IBox2& other) const = default;
  
    constexpr IBox2& operator+=(const IVec2<IntType>& iVec2)
    {
      min += iVec2;
      max += iVec2;
      return *this;
    }
    constexpr IBox2& operator-=(const IVec2<IntType>& iVec2) { return *this += -iVec2; }
    constexpr IBox2& operator+=(IntType n) { return *this += IBox2(n, n); }
    constexpr IBox2& operator-=(IntType n) { return *this -= IBox2(n, n); }
  
    constexpr IBox2& operator*=(IntType n)
    {
      min *= n;
      max *= n;
      return *this;
    }
    constexpr IBox2& operator/=(IntType n)
    {
      min /= n;
      max /= n;
      return *this;
    }
  
    constexpr IBox2 operator-() const { return IBox2(-min, -max); }
  
    constexpr IBox2 operator+(const IVec2<IntType>& iVec2) const { return Clone(*this) += iVec2; }
    constexpr IBox2 operator-(const IVec2<IntType>& iVec2) const { return Clone(*this) -= iVec2; }
  
    constexpr IBox2 operator+(IntType n) const { return Clone(*this) += n; }
    constexpr IBox2 operator-(IntType n) const { return Clone(*this) -= n; }
  
    constexpr IBox2 operator*(IntType n) const { return Clone(*this) *= n; }
    constexpr IBox2 operator/(IntType n) const { return Clone(*this) /= n; }
  
    /*
      \returns True if the box dimensions are non-negative.
    */
    constexpr bool valid() const { return min.i <= max.i && min.j <= max.j; }
  
    /*
      \returns True if the given point is contained within the box.
    */
    constexpr bool encloses(const IVec2<IntType>& iVec2) const
    {
      return iVec2.i >= min.i && iVec2.i <= max.i && iVec2.j >= min.j && iVec2.j <= max.j;
    }
  
    /*
      \returns The box dimensions.
    */
    constexpr IVec2<IntType> extents() const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
      return IVec2<IntType>(max.i - min.i + 1, max.j - min.j + 1);
    }
  
    /*
      \returns The number of integers contained within the box.
    */
    constexpr int volume() const
    {
      if (!valid())
        return 0;
  
      IVec2<IntType> boxExtents = extents();
      return boxExtents.i * boxExtents.j;
    }
  
    constexpr int linearIndexOf(const IVec2<IntType>& index) const
    {
      ENG_CORE_ASSERT(encloses(index), "Index is outside box!");
      IVec2<IntType> boxExtents = extents();
      IVec2<IntType> strides(boxExtents.j, 1);
      IVec2<IntType> indexRelativeToBase = index - min;
      return strides.dot(indexRelativeToBase);
    }
  
    constexpr IBox2& expand(IntType n = 1)
    {
      min -= n;
      max += n;
      return *this;
    }
    constexpr IBox2& shrink(IntType n = 1) { return expand(-n); }
  
    template<InvocableWithReturnType<bool, const IVec2<IntType>&> F>
    bool allOf(const F& condition) const
    {
      return noneOf([&condition](const IVec2<IntType>& index) { return !condition(index); });
    }
  
    template<InvocableWithReturnType<bool, const IVec2<IntType>&> F>
    bool anyOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec2<IntType> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          if (condition(index))
            return true;
      return false;
    }
  
    template<InvocableWithReturnType<bool, const IVec2<IntType>&> F>
    bool noneOf(const F& condition) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec2<IntType> index;
      for (index.i = min.i; index.i <= max.i; ++index.i)
        for (index.j = min.j; index.j <= max.j; ++index.j)
          if (condition(index))
            return false;
      return true;
    }
  
    template<InvocableWithReturnType<void, const IVec2<IntType>&> F>
    void forEach(const F& function) const
    {
      ENG_CORE_ASSERT(valid(), "Box is not valid!");
  
      IVec2<IntType> index;
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
      return { std::numeric_limits<IntType>::max(), std::numeric_limits<IntType>::min() };
    }
  };
}



namespace std
{
  template<std::integral IntType>
  inline ostream& operator<<(ostream& os, const eng::math::IBox2<IntType>& box)
  {
    return os << '(' << box.min << ", " << box.max << ')';
  }
}
