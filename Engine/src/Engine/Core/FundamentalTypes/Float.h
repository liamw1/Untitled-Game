#pragma once
#include "Int.h"

namespace eng
{
  namespace detail
  {
    template<typename T>
    concept ConvertibleType = std::is_arithmetic_v<T> || std::same_as<T, Int<typename T::UnderlyingType>>;

    template<typename T>
    concept Integral = std::integral<T> || std::same_as<T, Int<typename T::UnderlyingType>>;
  }

  template<std::floating_point T>
  class Float
  {
  public:
    using UnderlyingType = T;

    constexpr Float() = default;

    template<std::floating_point F>
    constexpr Float(F value)
      : m_Value(value) {}

    template<std::integral I>
    constexpr Float(I value)
      : m_Value(value) {}

    template<std::integral I>
    constexpr Float(Int<I> value)
      : m_Value(value.m_Value) {}

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



    template<std::integral I>
    constexpr operator I() const { return static_cast<I>(m_Value); }

    template<std::integral I>
    constexpr operator Int<I>() const { return Int<I>(m_Value); }

    template<std::floating_point F>
    constexpr operator F() const { return static_cast<F>(m_Value); }

    template<std::floating_point F>
    constexpr operator Float<F>() const { return Float<F>(m_Value); }



    template<detail::Integral I>
    constexpr Float operator*(I right) const { return *this * Float(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator*(F right) const { return *this * Float<F>(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator*(Float<F> right) const { return Float<largest<T, F>>(m_Value) *= right; }



    template<detail::Integral I>
    constexpr Float operator/(I right) const { return *this / Float(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator/(F right) const { return *this / Float<F>(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator/(Float<F> right) const { return Float<largest<T, F>>(m_Value) /= right; }



    template<detail::Integral I>
    constexpr Float operator+(I right) const { return *this + Float(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator+(F right) const { return *this + Float<F>(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator+(Float<F> right) const { return Float<largest<T, F>>(m_Value) += right; }



    template<detail::Integral I>
    constexpr Float operator-(I right) const { return *this - Float(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator-(F right) const { return *this - Float<F>(right); }

    template<std::floating_point F>
    constexpr Float<largest<T, F>> operator-(Float<F> right) const { return Float<largest<T, F>>(m_Value) -= right; }



    constexpr std::partial_ordering operator<=>(const Float& right) const = default;



    template<detail::Integral I>
    constexpr Float& operator+=(I right) { return *this += Float(right); }

    template<std::floating_point F>
    constexpr Float& operator+=(F right) { return *this += Float<F>(right); }

    template<std::floating_point F>
    constexpr Float& operator+=(Float<F> right)
    {
      m_Value += right.m_Value;
      return *this;
    }



    template<detail::Integral I>
    constexpr Float& operator-=(I right) { return *this -= Float(right); }

    template<std::floating_point F>
    constexpr Float& operator-=(F right) { return *this -= Float<F>(right); }

    template<std::floating_point F>
    constexpr Float& operator-=(Float<F> right) { return *this += -right; }



    template<detail::Integral I>
    constexpr Float& operator*=(I right) { return *this *= Float(right); }

    template<std::floating_point F>
    constexpr Float& operator*=(F right) { return *this *= Float<F>(right); }

    template<std::floating_point F>
    constexpr Float& operator*=(Float<F> right)
    {
      m_Value *= right.m_Value;
      return *this;
    }



    template<detail::Integral F>
    constexpr Float& operator/=(F right) { return *this /= Float(right); }

    template<std::floating_point F>
    constexpr Float& operator/=(F right) { return *this /= Float<F>(right); }

    template<std::floating_point F>
    constexpr Float& operator/=(Float<F> right)
    {
      m_Value /= right.m_Value; 
      return *this;
    }



    template<std::integral I>
    friend class Int;

  private:
    T m_Value;
  };



  template<detail::Integral I, std::floating_point F>
  constexpr Float<F> operator*(I left, Float<F> right) { return Float<F>(left) * right; }

  template<std::floating_point F1, std::floating_point F2>
  constexpr Float<largest<F1, F2>> operator*(F1 left, Float<F2> right) { return Float<F1>(left) * right; }



  template<detail::Integral I, std::floating_point F>
  constexpr Float<F> operator/(I left, Float<F> right) { return Float<F>(left) / right; }

  template<std::floating_point F1, std::floating_point F2>
  constexpr Float<largest<F1, F2>> operator/(F1 left, Float<F2> right) { return Float<F1>(left) / right; }



  template<detail::Integral I, std::floating_point F>
  constexpr Float<F> operator+(I left, Float<F> right) { return Float<F>(left) + right; }

  template<std::floating_point F1, std::floating_point F2>
  constexpr Float<largest<F1, F2>> operator+(F1 left, Float<F2> right) { return Float<F1>(left) + right; }



  template<detail::Integral I, std::floating_point F>
  constexpr Float<F> operator-(I left, Float<F> right) { return Float<F>(left) - right; }

  template<std::floating_point F1, std::floating_point F2>
  constexpr Float<largest<F1, F2>> operator-(F1 left, Float<F2> right) { return Float<F1>(left) - right; }
}