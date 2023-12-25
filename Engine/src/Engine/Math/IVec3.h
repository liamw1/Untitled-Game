#pragma once
#include "IVec2.h"

namespace eng::math
{
  /*
    Represents a point on a 3D integer lattice.
  */
  template<std::integral T>
  struct IVec3
  {
    T i;
    T j;
    T k;
  
    constexpr IVec3()
      : IVec3(0) {}
    constexpr IVec3(T n)
      : IVec3(n, n, n) {}
    constexpr IVec3(T _i, T _j, T _k)
      : i(_i), j(_j), k(_k) {}
    constexpr IVec3(IVec2<T> intVec2, T _k)
      : i(intVec2.i), j(intVec2.j), k(_k) {}
    constexpr IVec3(EnumBitMask<Axis> bitMask)
      : i(bitMask[Axis::X]), j(bitMask[Axis::Y]), k(bitMask[Axis::Z]) {}
  
    explicit constexpr operator Vec2() const { return Vec2(i, j); }
    explicit constexpr operator Vec3() const { return Vec3(i, j, k); }
    explicit constexpr operator IVec2<T>() const { return IVec2<T>(i, j); }

    template<std::integral U>
    constexpr IVec3<U> upcast() const { return { arithmeticUpcast<U>(i), arithmeticUpcast<U>(j), arithmeticUpcast<U>(k) }; }

    template<std::integral U>
    constexpr IVec3<U> checkedCast() const { return { arithmeticCast<U>(i), arithmeticCast<U>(j), arithmeticCast<U>(k) }; }

    template<std::integral U>
    constexpr IVec3<U> uncheckedCast() const { return { arithmeticCastUnchecked<U>(i), arithmeticCastUnchecked<U>(j), arithmeticCastUnchecked<U>(k) }; }
  
    constexpr T& operator[](Axis axis) { ENG_MUTABLE_VERSION(operator[], axis); }
    constexpr const T& operator[](Axis axis) const
    {
      switch (axis)
      {
        case Axis::X: return i;
        case Axis::Y: return j;
        case Axis::Z: return k;
      }
      throw CoreException("Invalid axis!");
    }
  
    // Define lexicographical ordering on 3D indices
    constexpr std::strong_ordering operator<=>(const IVec3& other) const = default;
  
    constexpr IVec3& operator+=(const IVec3& other)
    {
      i += other.i;
      j += other.j;
      k += other.k;
      return *this;
    }
    constexpr IVec3& operator-=(const IVec3& other) { return *this += -other; }
    constexpr IVec3& operator+=(T n) { return *this += IVec3(n); }
    constexpr IVec3& operator-=(T n) { return *this -= IVec3(n); }
  
    constexpr IVec3& operator*=(T n)
    {
      ENG_CORE_ASSERT(debug::productOverflowCheck(n, i) && debug::productOverflowCheck(n, j) && debug::productOverflowCheck(n, k), "Integer overflow!");
      i *= n;
      j *= n;
      k *= n;
      return *this;
    }
    constexpr IVec3& operator/=(T n)
    {
      i /= n;
      j /= n;
      k /= n;
      return *this;
    }
  
    constexpr IVec3 operator-() const { return IVec3(-i, -j, -k); };
  
    constexpr IVec3 operator+(IVec3 other) const { return other += *this; }
    constexpr IVec3 operator-(IVec3 other) const { return -(other -= *this); }
  
    constexpr IVec3 operator+(T n) const { return clone(*this) += n; }
    constexpr IVec3 operator-(T n) const { return clone(*this) -= n; }
  
    constexpr IVec3 operator*(T n) const { return clone(*this) *= n; }
    constexpr IVec3 operator/(T n) const { return clone(*this) /= n; }
  
    constexpr T l1Norm() const { return std::abs(i) + std::abs(j) + std::abs(k); }
    constexpr T dot(const IVec3& other) const { return i * other.i + j * other.j + k * other.k; }
  
    static constexpr IVec3 ToIndex(const Vec3& vec)
    {
      return IVec3(arithmeticCast<T>(std::floor(vec.x)), arithmeticCast<T>(std::floor(vec.y)), arithmeticCast<T>(std::floor(vec.z)));
    }
  
    static constexpr IVec3 Dir(Direction direction)
    {
      T value = isUpstream(direction) ? 1 : -1;
      return CreatePermuted(value, 0, 0, axisOf(direction));
    }
  
    static constexpr IVec3 CreatePermuted(T i, T j, T k, Axis permutation)
    {
      switch (permutation)
      {
        case Axis::X: return IVec3(i, j, k);
        case Axis::Y: return IVec3(k, i, j);
        case Axis::Z: return IVec3(j, k, i);
      }
      throw CoreException("Invalid permutation!");
    }
  };
  
  template<std::integral T>
  constexpr IVec3<T> operator*(T n, IVec3<T> index)
  {
    return index *= n;
  }
  
  template<std::integral T>
  constexpr IVec3<T> ComponentWiseMin(const IVec3<T>& a, const IVec3<T>& b)
  {
    return IVec3(std::min(a.i, b.i), std::min(a.j, b.j), std::min(a.k, b.k));
  }
  
  template<std::integral T>
  constexpr IVec3<T> ComponentWiseMax(const IVec3<T>& a, const IVec3<T>& b)
  {
    return IVec3(std::max(a.i, b.i), std::max(a.j, b.j), std::max(a.k, b.k));
  }
}



namespace std
{
  template<std::integral T>
  inline ostream& operator<<(ostream& os, const eng::math::IVec3<T>& index)
  {
    using promotedType = std::conditional_t<std::is_signed_v<T>, iMax, uMax>;
    return os << '[' << eng::arithmeticUpcast<promotedType>(index.i) << ", "
                     << eng::arithmeticUpcast<promotedType>(index.j) << ", "
                     << eng::arithmeticUpcast<promotedType>(index.k) << ']';
  }

  template<std::integral T>
  struct hash<eng::math::IVec3<T>>
  {
    constexpr uSize operator()(const eng::math::IVec3<T>& index) const
    {
      constexpr i32 n = std::numeric_limits<uSize>::digits / 3; 
      constexpr uSize stride = eng::math::pow2<uSize>(n) - 1;
      constexpr eng::math::IVec3<uSize> strides(1, stride, eng::math::square(stride));

      return strides.i * eng::math::mod<stride>(index.i) +
             strides.j * eng::math::mod<stride>(index.j) +
             strides.k * eng::math::mod<stride>(index.k);
    }
  };
}