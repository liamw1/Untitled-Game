﻿#pragma once
#include "Helpers.h"
#include "BoilerplateReduction.h"
#include "Engine/Core/Algorithm.h"

namespace eng
{
  // C++23: This can be replaced with std::to_underlying
  template<Enum E>
  constexpr std::underlying_type_t<E> toUnderlying(E e) { return static_cast<std::underlying_type_t<E>>(e); }

  template<IterableEnum E>
  constexpr i32 enumRange()
  {
    static_assert(toUnderlying(E::Last) > toUnderlying(E::First), "First and Last enums are in incorrect order!");
    return 1 + toUnderlying(E::Last) - toUnderlying(E::First);
  }

  // ==================== Enabling Iteration for Enum Classes ==================== //
  // Code borrowed from: https://stackoverflow.com/questions/261963/how-can-i-iterate-over-an-enum
  template<IterableEnum E>
  class EnumIterator
  {
    std::underlying_type_t<E> m_Value;

  public:
    constexpr EnumIterator()
      : EnumIterator(E::First) {}
    constexpr EnumIterator(E valEnum)
      : m_Value(toUnderlying(valEnum)) {}

    constexpr EnumIterator& operator++()
    {
      ++m_Value;
      return *this;
    }
    constexpr E operator*() const { return static_cast<E>(m_Value); }
    constexpr bool operator!=(const EnumIterator& other) const { return m_Value != other.m_Value; }

    constexpr EnumIterator begin() const { return EnumIterator(E::First); }
    constexpr EnumIterator next() const { return ++clone(*this); }
    constexpr EnumIterator end() const { return ++EnumIterator(E::Last); }
  };

  // ===================== Arrays Indexable by Enum Classes ====================== //
  template<typename T, IterableEnum E>
  class EnumArray
  {
    std::array<T, enumRange<E>()> m_Data;

  public:
    constexpr EnumArray() = default;
    constexpr EnumArray(const T& initialValue) { algo::fill(m_Data, initialValue); }
    constexpr EnumArray(const std::initializer_list<std::pair<E, T>>& list)
    {
      for (const auto& [index, value] : list)
        operator[](index) = value;
    }

    constexpr T& operator[](E index) { return ENG_MUTABLE_VERSION(operator[], index); }
    constexpr const T& operator[](E index) const { return m_Data[toUnderlying(index) - toUnderlying(E::First)]; }

    constexpr size_t size() { return m_Data.size(); }

    ENG_DEFINE_CONSTEXPR_ITERATORS(m_Data);
  };
}



// ==================== Enabling Bitmasking for Enum Classes ==================== //
#define ENG_ENABLE_BITMASK_OPERATORS(x)  \
template<>                              \
struct EnableBitMaskOperators<x> { static const bool enable = true; };

template<typename E>
struct EnableBitMaskOperators { static const bool enable = false; };

template<typename E>
typename std::enable_if<EnableBitMaskOperators<E>::enable, E>::type
  operator&(E enumA, E enumB)
{
  return static_cast<E>(eng::toUnderlying(enumA) & eng::toUnderlying(enumB));
}

template<typename E>
typename std::enable_if<EnableBitMaskOperators<E>::enable, E>::type
  operator|(E enumA, E enumB)
{
  return static_cast<E>(eng::toUnderlying(enumA) | eng::toUnderlying(enumB));
}