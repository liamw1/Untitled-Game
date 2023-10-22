#pragma once
#include "Engine/Utilities/Helpers.h"

namespace eng
{
  template<std::floating_point T>
  class Float;

  template<std::integral T>
  class Int
  {
  public:
    constexpr Int() = default;

    template<std::integral U>
    constexpr Int(U value)
      : m_Value(value) {}

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



    template<std::integral U>
    constexpr Int<largest<T, U>> operator*(U other) const { return *this * Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator*(Int<U> other) const { return Int<largest<T, U>>(m_Value) *= other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator/(U other) const { return *this / Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator/(Int<U> other) const { return Int<largest<T, U>>(m_Value) /= other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator%(U other) const { return *this % Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator%(Int<U> other) const { return Int<largest<T, U>>(m_Value) %= other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator+(U other) const { return *this + Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator+(Int<U> other) const { return Int<largest<T, U>>(m_Value) += other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator-(U other) const { return *this - Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator-(Int<U> other) const { return Int<largest<T, U>>(m_Value) -= other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator<<(U other) const { return *this << Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator<<(Int<U> other) const { return Int<largest<T, U>>(m_Value) <<= other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator>>(U other) const { return *this >> Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator>>(Int<U> other) const { return Int<largest<T, U>>(m_Value) >>= other; }



    constexpr std::strong_ordering operator<=>(const Int& other) const = default;



    template<std::integral U>
    constexpr Int<largest<T, U>> operator&(U other) const { return *this & Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator&(Int<U> other) const { return Int<largest<T, U>>(m_Value) &= other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator^(U other) const { return *this ^ Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator^(Int<U> other) const { return Int<largest<T, U>>(m_Value) ^= other; }



    template<std::integral U>
    constexpr Int<largest<T, U>> operator|(U other) const { return *this | Int<U>(other); }

    template<std::integral U>
    constexpr Int<largest<T, U>> operator|(Int<U> other) const { return Int<largest<T, U>>(m_Value) |= other; }



    template<std::integral U>
    constexpr Int& operator+=(U other) { return *this += Int(other); }

    template<std::integral U>
    constexpr Int& operator+=(Int<U> other)
    {
      m_Value += other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator-=(U other) { return *this -= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator-=(Int<U> other) { return *this += -other; }



    template<std::integral U>
    constexpr Int& operator*=(U other) { return *this *= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator*=(Int<U> other)
    {
      m_Value *= other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator/=(U other) { return *this /= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator/=(Int<U> other)
    {
      m_Value /= other.m_Value; 
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator%=(U other) { return *this %= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator%=(Int<U> other)
    {
      m_Value %= other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator<<=(U other) { return *this <<= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator<<=(Int<U> other)
    {
      m_Value <<= other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator>>=(U other) { return *this >>= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator>>=(Int<U> other)
    {
      m_Value >>= other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator&=(U other) { return *this &= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator&=(Int<U> other)
    {
      m_Value &= other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator^=(U other) { return *this ^= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator^=(Int<U> other)
    {
      m_Value ^= other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Int& operator|=(U other) { return *this |= Int<U>(other); }

    template<std::integral U>
    constexpr Int& operator|=(Int<U> other)
    {
      m_Value |= other.m_Value;
      return *this;
    }



    template<std::floating_point U>
    friend class Float;

  private:
    T m_Value;
  };
}