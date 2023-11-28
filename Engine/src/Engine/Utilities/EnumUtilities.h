#pragma once
#include "Helpers.h"
#include "BoilerplateReduction.h"
#include "Engine/Core/Algorithm.h"

namespace eng
{
  // C++23: This can be replaced with std::to_underlying
  template<Enum E>
  constexpr std::underlying_type_t<E> toUnderlying(E e) { return static_cast<std::underlying_type_t<E>>(e); }

  template<IterableEnum E>
  constexpr std::underlying_type_t<E> enumRange()
  {
    static_assert(toUnderlying(E::Last) > toUnderlying(E::First), "First and Last enums are in incorrect order!");
    return 1 + toUnderlying(E::Last) - toUnderlying(E::First);
  }

  // ==================== Enabling Iteration for Enum Classes ==================== //
  template<IterableEnum E>
  class EnumIterator
  {
  public:
    // These aliases are needed to satisfy requirements of std::forward_iterator
    using value_type = E;
    using difference_type = std::underlying_type_t<E>;

  private:
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
    constexpr EnumIterator operator++(int)
    {
      EnumIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr E operator*() const { return enumCastUnchecked<E>(m_Value); }

    constexpr std::strong_ordering operator<=>(const EnumIterator& other) const = default;

    constexpr EnumIterator begin() const { return EnumIterator(E::First); }
    constexpr EnumIterator next() const { return ++clone(*this); }
    constexpr EnumIterator end() const { return ++EnumIterator(E::Last); }
  };

  // ===================== Arrays Indexable by Enum Classes ====================== //
  template<typename T, IterableEnum E>
  class EnumArray
  {
    std::array<T, enumRange<E>()> m_Data{};

  public:
    constexpr EnumArray() = default;
    constexpr EnumArray(const T& initialValue) { m_Data.fill(initialValue); }
    constexpr EnumArray(const std::initializer_list<std::pair<E, T>>& list)
    {
      std::array<i32, enumRange<E>()> initializationCounts{};
      for (const auto& [enumIndex, value] : list)
      {
        std::underlying_type_t<E> arrayIndex = toUnderlying(enumIndex);
        m_Data[arrayIndex] = value;
        initializationCounts[arrayIndex]++;
      }

      auto badInitializationPosition = algo::findIf(initializationCounts, [](i32 count) { return count != 1; });
      if (badInitializationPosition != initializationCounts.end())
      {
        i32 count = *badInitializationPosition;
        if (count == 0)
          throw std::runtime_error("Not all values have been initialized!");
        else
          throw std::runtime_error("A value has been initialized more than once!");
      }
    }

    ENG_DEFINE_CONSTEXPR_ITERATORS(m_Data);

    constexpr T& operator[](E enumIndex) { ENG_MUTABLE_VERSION(operator[], enumIndex); }
    constexpr const T& operator[](E enumIndex) const { return m_Data[toUnderlying(enumIndex) - toUnderlying(E::First)]; }

    constexpr uSize size() const { return m_Data.size(); }
    constexpr const T* data() const { return m_Data.data(); }
  };
}



// ==================== Enabling Bitmasking for Enum Classes ==================== //
#define ENG_ENABLE_BITMASK_OPERATORS(x) \
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