#pragma once
#include "Helpers.h"

// ==================== Enabling Bitmasking for Enum Classes ==================== //
#define EN_ENABLE_BITMASK_OPERATORS(x)  \
template<>                              \
struct EnableBitMaskOperators<x> { static const bool enable = true; };

template<typename Enum>
struct EnableBitMaskOperators { static const bool enable = false; };

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
  operator&(Enum enumA, Enum enumB)
{
  return static_cast<Enum>(static_cast<std::underlying_type<Enum>::type>(enumA) &
    static_cast<std::underlying_type<Enum>::type>(enumB));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type
  operator|(Enum enumA, Enum enumB)
{
  return static_cast<Enum>(static_cast<std::underlying_type<Enum>::type>(enumA) |
    static_cast<std::underlying_type<Enum>::type>(enumB));
}

namespace eng
{
  template<IterableEnum E>
  constexpr int enumRange()
  {
    EN_CORE_ASSERT(static_cast<int>(E::End) > static_cast<int>(E::Begin), "Begin and End enums are in incorrect order!");
    return 1 + static_cast<int>(E::End) - static_cast<int>(E::Begin);
  }

  // ==================== Enabling Iteration for Enum Classes ==================== //
  // Code borrowed from: https://stackoverflow.com/questions/261963/how-can-i-iterate-over-an-enum
  template<IterableEnum E>
  class EnumIterator
  {
  public:
    constexpr EnumIterator()
      : EnumIterator(E::Begin) {}
    constexpr EnumIterator(E valEnum)
      : m_Value(std::underlying_type_t<E>(valEnum)) {}

    constexpr EnumIterator& operator++()
    {
      ++m_Value;
      return *this;
    }
    constexpr E operator*() const { return static_cast<E>(m_Value); }
    constexpr bool operator!=(const EnumIterator& other) const { return m_Value != other.m_Value; }

    constexpr EnumIterator begin() const { return EnumIterator(E::Begin); }
    constexpr EnumIterator next() const { return ++clone(*this); }
    constexpr EnumIterator end() const { return ++EnumIterator(E::End); }

  private:
    std::underlying_type_t<E> m_Value;
  };

  // ===================== Arrays Indexable by Enum Classes ====================== //
  template<typename T, IterableEnum E>
  class EnumArray
  {
  public:
    constexpr EnumArray() = default;
    constexpr EnumArray(const T& initialValue) { std::fill(m_Data.begin(), m_Data.end(), initialValue); }
    constexpr EnumArray(const std::initializer_list<T>& list)
    {
      if (list.size() == m_Data.size())
        std::copy(list.begin(), list.end(), m_Data.begin());
      else
        EN_CORE_ERROR("Initializer list is incorrect size!");
    }

    constexpr T& operator[](E index)
    {
      return const_cast<T&>(static_cast<const EnumArray*>(this)->operator[](index));
    }
    constexpr const T& operator[](E index) const
    {
      return m_Data[std::underlying_type_t<E>(index)];
    }

    constexpr size_t size() { return m_Data.size(); }

  private:
    std::array<T, enumRange<E>()> m_Data;
  };
}