#pragma once
#include "Int.h"

namespace eng
{
  template<std::floating_point T>
  class Float
  {
  public:
    constexpr Float() = default;

    template<std::integral U>
    constexpr Float(U value)
      : m_Value(value) {}

    template<std::integral U>
    constexpr Float(Int<U> value)
      : m_Value(value.m_Value) {}

    template<std::floating_point U>
    constexpr Float(U value)
      : m_Value(value) {}

    // Operators are defined in order of operator precedence https://en.cppreference.com/w/cpp/language/operator_precedence

    constexpr Float operator++(int /*unused*/)
    {
      Float result = *this;
      operator++();
      return result;
    }

    constexpr Float operator--(int /*unused*/)
    {
      Float result = *this;
      operator--();
      return result;
    }

    constexpr Float& operator++()
    {
      ++m_Value;
      return *this;
    }

    constexpr Float& operator--()
    {
      --m_Value;
      return *this;
    }

    constexpr Float<decltype(+std::declval<T>())> operator+() const { return Float<decltype(+m_Value)>(+m_Value); }
    constexpr Float operator-() const { return Float(-m_Value); }

    constexpr bool operator!() const { return !m_Value; }

    template<std::integral U>
    constexpr Float operator*(U other) const { return *this * Float(other); }

    template<std::integral U>
    constexpr Float operator*(Int<U> other) const { return *this * Float(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator*(U other) const { return *this * Float<U>(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator*(Float<U> other) const { return Float<largest<T, U>>(m_Value) *= other; }



    template<std::integral U>
    constexpr Float operator/(U other) const { return *this / Float(other); }

    template<std::integral U>
    constexpr Float operator/(Int<U> other) const { return *this / Float(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator/(U other) const { return *this / Float<U>(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator/(Float<U> other) const { return Float<largest<T, U>>(m_Value) /= other; }



    template<std::integral U>
    constexpr Float operator+(U other) const { return *this + Float(other); }

    template<std::integral U>
    constexpr Float operator+(Int<U> other) const { return *this + Float(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator+(U other) const { return *this + Float<U>(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator+(Float<U> other) const { return Float<largest<T, U>>(m_Value) += other; }



    template<std::integral U>
    constexpr Float operator-(U other) const { return *this - Float(other); }

    template<std::integral U>
    constexpr Float operator-(Int<U> other) const { return *this - Float(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator-(U other) const { return *this - Float<U>(other); }

    template<std::floating_point U>
    constexpr Float<largest<T, U>> operator-(Float<U> other) const { return Float<largest<T, U>>(m_Value) -= other; }



    constexpr std::partial_ordering operator<=>(const Float& other) const = default;



    template<std::integral U>
    constexpr Float& operator+=(U other) { return *this += Float(other); }

    template<std::integral U>
    constexpr Float& operator+=(Int<U> other) { return *this += Float(other); }

    template<std::floating_point U>
    constexpr Float& operator+=(U other) { return *this += Float<U>(other); }

    template<std::floating_point U>
    constexpr Float& operator+=(Float<U> other)
    {
      m_Value += other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Float& operator-=(U other) { return *this -= Float(other); }

    template<std::integral U>
    constexpr Float& operator-=(Int<U> other) { return *this -= Float(other); }

    template<std::floating_point U>
    constexpr Float& operator-=(U other) { return *this -= Float<U>(other); }

    template<std::floating_point U>
    constexpr Float& operator-=(Float<U> other) { return *this += -other; }



    template<std::integral U>
    constexpr Float& operator*=(U other) { return *this *= Float(other); }

    template<std::integral U>
    constexpr Float& operator*=(Int<U> other) { return *this *= Float(other); }

    template<std::floating_point U>
    constexpr Float& operator*=(U other) { return *this *= Float<U>(other); }

    template<std::floating_point U>
    constexpr Float& operator*=(Float<U> other)
    {
      m_Value *= other.m_Value;
      return *this;
    }



    template<std::integral U>
    constexpr Float& operator/=(U other) { return *this /= Float(other); }

    template<std::integral U>
    constexpr Float& operator/=(Int<U> other) { return *this /= Float(other); }

    template<std::floating_point U>
    constexpr Float& operator/=(U other) { return *this /= Float<U>(other); }

    template<std::floating_point U>
    constexpr Float& operator/=(Float<U> other)
    {
      m_Value /= other.m_Value; 
      return *this;
    }

  private:
    T m_Value;
  };
}