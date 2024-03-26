#pragma once
#include "IVec2.h"
#include "Engine/Core/Concepts.h"

namespace eng::math
{
  /*
    Represents a box on a 2D integer lattice. Min and max bounds are both inclusive.
  */
  template<std::integral T>
  class IBox2
  {
  public:
    class iterator
    {
    public:
      // These aliases are needed to satisfy requirements of std::forward_iterator
      using value_type = IVec2<T>;
      using difference_type = T;

    private:
      const IBox2* m_Box;
      value_type m_Index;

    public:
      constexpr iterator()
        : m_Box(nullptr) {}
      constexpr iterator(const IBox2* box, value_type index)
        : m_Box(box), m_Index(index) {}

      constexpr iterator& operator++()
      {
        if (m_Index.j < m_Box->max.j)
          m_Index.j++;
        else
        {
          m_Index.j = m_Box->min.j;
          m_Index.i++;
        }
        return *this;
      }
      constexpr iterator operator++(int)
      {
        iterator tmp = *this;
        ++(*this);
        return tmp;
      }

      constexpr const value_type& operator*() const { return m_Index; }

      constexpr std::strong_ordering operator<=>(const iterator& other) const { return m_Index <=> other.m_Index; }
      constexpr bool operator==(const iterator& other) const { return m_Index == other.m_Index; }
    };

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

    template<std::integral U>
    constexpr IBox2<U> uncheckedCast() const { return { min.uncheckedCast<U>(), max.uncheckedCast<U>() }; }
  
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

    constexpr iterator begin() const { return iterator(this, min); }
    constexpr iterator end() const
    {
      ENG_CORE_ASSERT(max.i < std::numeric_limits<T>::max(), "Box is too large to be iterated over!");
      IVec2<T> endValue(max.i + 1, min.j);
      return iterator(this, endValue);
    }
  
    /*
      \returns True if the box dimensions are non-negative.
    */
    constexpr bool valid() const { return min.i <= max.i && min.j <= max.j; }

    /*
      \returns True if the box has a non-zero volume.
    */
    constexpr bool empty() const { return min.i == max.i && min.j == max.j; }
  
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

    constexpr IBox2& flooredDivide(T n)
    {
      min.flooredDivide(n);
      max.flooredDivide(n);
      return *this;
    }

    bool intersects(const IBox2& other) const
    {
      IBox2 intersection = Intersection(*this, other);
      return intersection.valid() && !intersection.empty();
    }
  
    constexpr IBox2& expand(T n = 1)
    {
      min -= n;
      max += n;
      return *this;
    }
    constexpr IBox2& shrink(T n = 1) { return expand(-n); }
  
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
