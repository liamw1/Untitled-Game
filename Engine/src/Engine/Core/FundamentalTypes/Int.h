#pragma once
#include "Engine/Utilities/Helpers.h"

namespace eng
{
  template<std::floating_point F>
  class Float;

  template<std::integral T>
  class Int
  {
  public:
    using UnderlyingType = T;

    constexpr Int() = default;

    template<std::integral I>
    constexpr Int(I value)
      : m_Value(value) {}

    template<std::floating_point F>
    constexpr Int(F value)
      : m_Value(value) {}

    template<std::floating_point F>
    constexpr Int(Float<F> value)
      : m_Value(value.m_Value) {}

    // Operators are defined in order of operator precedence https://en.cppreference.com/w/cpp/language/operator_precedence

    constexpr Int operator++(int /*unused*/)
    {
      Int result = *this;
      operator++();
      return result;
    }

    constexpr Int operator--(int /*unused*/)
    {
      Int result = *this;
      operator--();
      return result;
    }

    constexpr Int& operator++()
    {
      ++m_Value;
      return *this;
    }

    constexpr Int& operator--()
    {
      --m_Value;
      return *this;
    }

    constexpr Int<decltype(+std::declval<T>())> operator+() const { return Int<decltype(+m_Value)>(+m_Value); }
    constexpr Int operator-() const { return Int(-m_Value); }

    constexpr bool operator!() const { return !m_Value; }
    constexpr Int operator~() const { return ~m_Value; }



    template<std::integral I>
    constexpr operator I() const { return static_cast<I>(m_Value); }

    template<std::integral I>
    constexpr operator Int<I>() const { return Int<I>(m_Value); }

    template<std::floating_point F>
    constexpr operator F() const { return static_cast<F>(m_Value); }

    template<std::floating_point F>
    constexpr operator Float<F>() const { return Float<F>(m_Value); }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator*(I right) const { return *this * Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator*(Int<I> right) const { return Int<largest<T, I>>(m_Value) *= right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator/(I right) const { return *this / Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator/(Int<I> right) const { return Int<largest<T, I>>(m_Value) /= right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator%(I right) const { return *this % Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator%(Int<I> right) const { return Int<largest<T, I>>(m_Value) %= right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator+(I right) const { return *this + Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator+(Int<I> right) const { return Int<largest<T, I>>(m_Value) += right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator-(I right) const { return *this - Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator-(Int<I> right) const { return Int<largest<T, I>>(m_Value) -= right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator<<(I right) const { return *this << Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator<<(Int<I> right) const { return Int<largest<T, I>>(m_Value) <<= right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator>>(I right) const { return *this >> Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator>>(Int<I> right) const { return Int<largest<T, I>>(m_Value) >>= right; }



    constexpr std::strong_ordering operator<=>(const Int& right) const = default;



    template<std::integral I>
    constexpr Int<largest<T, I>> operator&(I right) const { return *this & Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator&(Int<I> right) const { return Int<largest<T, I>>(m_Value) &= right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator^(I right) const { return *this ^ Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator^(Int<I> right) const { return Int<largest<T, I>>(m_Value) ^= right; }



    template<std::integral I>
    constexpr Int<largest<T, I>> operator|(I right) const { return *this | Int<I>(right); }

    template<std::integral I>
    constexpr Int<largest<T, I>> operator|(Int<I> right) const { return Int<largest<T, I>>(m_Value) |= right; }



    template<std::integral I>
    constexpr Int& operator+=(I right) { return *this += Int(right); }

    template<std::integral I>
    constexpr Int& operator+=(Int<I> right)
    {
      m_Value += right.m_Value;
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator-=(I right) { return *this -= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator-=(Int<I> right) { return *this += -right; }



    template<std::integral I>
    constexpr Int& operator*=(I right) { return *this *= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator*=(Int<I> right)
    {
      m_Value *= right.m_Value;
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator/=(I right) { return *this /= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator/=(Int<I> right)
    {
      m_Value /= right.m_Value; 
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator%=(I right) { return *this %= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator%=(Int<I> right)
    {
      m_Value %= right.m_Value;
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator<<=(I right) { return *this <<= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator<<=(Int<I> right)
    {
      m_Value <<= right.m_Value;
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator>>=(I right) { return *this >>= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator>>=(Int<I> right)
    {
      m_Value >>= right.m_Value;
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator&=(I right) { return *this &= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator&=(Int<I> right)
    {
      m_Value &= right.m_Value;
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator^=(I right) { return *this ^= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator^=(Int<I> right)
    {
      m_Value ^= right.m_Value;
      return *this;
    }



    template<std::integral I>
    constexpr Int& operator|=(I right) { return *this |= Int<I>(right); }

    template<std::integral I>
    constexpr Int& operator|=(Int<I> right)
    {
      m_Value |= right.m_Value;
      return *this;
    }



    template<std::floating_point F>
    friend class Float;

  private:
    T m_Value;
  };



  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator*(I1 left, Int<I2> right) { return Int<I1>(left) * right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator/(I1 left, Int<I2> right) { return Int<I1>(left) / right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator%(I1 left, Int<I2> right) { return Int<I1>(left) % right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator+(I1 left, Int<I2> right) { return Int<I1>(left) + right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator-(I1 left, Int<I2> right) { return Int<I1>(left) - right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator<<(I1 left, Int<I2> right) { return Int<I1>(left) << right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator>>(I1 left, Int<I2> right) { return Int<I1>(left) >> right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator&(I1 left, Int<I2> right) { return Int<I1>(left) & right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator^(I1 left, Int<I2> right) { return Int<I1>(left) ^ right; }

  template<std::integral I1, std::integral I2>
  constexpr Int<largest<I1, I2>> operator|(I1 left, Int<I2> right) { return Int<I1>(left) | right; }
}