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
  
    explicit constexpr operator Vec2() const { return Vec2(i, j); }
    explicit constexpr operator Vec3() const { return Vec3(i, j, k); }
  
    template<std::integral U>
    explicit constexpr operator IVec2<U>() const { return { static_cast<U>(i), static_cast<U>(j) }; }
  
    template<std::integral U>
    explicit constexpr operator IVec3<U>() const { return { static_cast<U>(i), static_cast<U>(j), static_cast<U>(k) }; }
  
    constexpr T& operator[](Axis axis) { return ENG_MUTABLE_VERSION(operator[], axis); }
    constexpr const T& operator[](Axis axis) const
    {
      switch (axis)
      {
        case Axis::X: return i;
        case Axis::Y: return j;
        case Axis::Z: return k;
        default:      throw std::invalid_argument("Invalid axis!");
      }
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
      ENG_CORE_ASSERT(debug::OverflowCheck(n, i) && debug::OverflowCheck(n, j) && debug::OverflowCheck(n, k), "Integer overflow!");
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
  
    constexpr IVec3 operator*(T n) const { clone(*this) *= n; }
    constexpr IVec3 operator/(T n) const { clone(*this) /= n; }
  
    constexpr int l1Norm() const { return std::abs(i) + std::abs(j) + std::abs(k); }
    constexpr int dot(const IVec3& other) const { return i * other.i + j * other.j + k * other.k; }
  
    static constexpr IVec3 ToIndex(const Vec3& vec)
    {
      return IVec3(static_cast<T>(std::floor(vec.x)), static_cast<T>(std::floor(vec.y)), static_cast<T>(std::floor(vec.z)));
    }
  
    static constexpr const IVec3& Dir(Direction direction)
    {
      static constexpr EnumArray<IVec3, Direction> directions = {{Direction::West,   IVec3(-1, 0, 0)}, {Direction::East,  IVec3(1, 0, 0)},
                                                                 {Direction::South,  IVec3(0, -1, 0)}, {Direction::North, IVec3(0, 1, 0)},
                                                                 {Direction::Bottom, IVec3(0, 0, -1)}, {Direction::Top,   IVec3(0, 0, 1)}};
      return directions[direction];
    }
  
    static constexpr IVec3 CreatePermuted(T i, T j, T k, Axis permutation)
    {
      switch (permutation)
      {
        case Axis::X: return IVec3(i, j, k);
        case Axis::Y: return IVec3(k, i, j);
        case Axis::Z: return IVec3(j, k, i);
        default:      throw std::invalid_argument("Invalid permutation!");
      }
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
    return os << '[' << static_cast<int64_t>(index.i) << ", " << static_cast<int64_t>(index.j) << ", " << static_cast<int64_t>(index.k) << ']';
  }

  template<std::integral T>
  struct hash<eng::math::IVec3<T>>
  {
    constexpr int operator()(const eng::math::IVec3<T>& index) const
    {
      return eng::u32Bit( 0) * (index.i % eng::u32Bit(10)) +
             eng::u32Bit(10) * (index.j % eng::u32Bit(10)) +
             eng::u32Bit(20) * (index.k % eng::u32Bit(10));
    }
  };
}