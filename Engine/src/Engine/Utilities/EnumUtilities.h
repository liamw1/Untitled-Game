#pragma once
#include "BitUtilities.h"
#include "Helpers.h"
#include "BoilerplateReduction.h"
#include "Engine/Core/Algorithm.h"

namespace eng
{
  // C++23: This can be replaced with std::to_underlying
  template<Enum E>
  constexpr std::underlying_type_t<E> toUnderlying(E e) { return static_cast<std::underlying_type_t<E>>(e); }

  template<IterableEnum E>
  constexpr std::underlying_type_t<E> enumIndex(E e) { return toUnderlying(e) - toUnderlying(E::First); }

  template<IterableEnum E>
  constexpr std::underlying_type_t<E> enumCount() { return enumIndex(E::Last) + 1; }

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

  /*
    A simple wrapper over a std::array that is indexed into using an enum rather than an integer.
  */
  template<typename T, IterableEnum E>
  class EnumArray
  {
    std::array<T, enumCount<E>()> m_Data{};

  public:
    constexpr EnumArray() = default;

    template<typename... Args>
      requires std::constructible_from<T, Args...>
    constexpr EnumArray(Args&&... args) { m_Data.fill(T(args...)); }

    constexpr EnumArray(const std::initializer_list<std::pair<E, T>>& initializerList)
    {
      std::array<i32, enumCount<E>()> initializationCounts{};
      for (const auto& [e, value] : initializerList)
      {
        std::underlying_type_t<E> arrayIndex = enumIndex(e);
        m_Data[arrayIndex] = value;
        initializationCounts[arrayIndex]++;
      }

      auto badInitializationPosition = algo::findIf(initializationCounts, [](i32 count) { return count != 1; });
      if (badInitializationPosition != initializationCounts.end())
      {
        i32 count = *badInitializationPosition;
        std::string_view message = count == 0 ? "Not all values have been initialized!" : "A value has been initialized more than once!";
        throw CoreException(message);
      }
    }

    ENG_DEFINE_CONSTEXPR_ITERATORS(m_Data);

    constexpr T& operator[](E e) { ENG_MUTABLE_VERSION(operator[], e); }
    constexpr const T& operator[](E e) const { return m_Data[enumIndex(e)]; }

    constexpr T& front() { ENG_MUTABLE_VERSION(front); }
    constexpr const T& front() const { return m_Data.front(); }

    constexpr T& back() { ENG_MUTABLE_VERSION(back); }
    constexpr const T& back() const { return m_Data.back(); }

    constexpr uSize size() const { return m_Data.size(); }
    constexpr const T* data() const { return m_Data.data(); }

    constexpr E enumFromIterator(iterator it) const { return static_cast<E>(it - m_Data.begin()); }
    constexpr E enumFromIterator(const_iterator it) const { return static_cast<E>(it - m_Data.begin()); }
  };

  /*
    A class used for storing boolean values associated with an enum.
    It currently only supports enums of size <= 8, but can easily be
    extended to sizes of up to 64 if ever needed.
  */
  template<IterableEnum E>
  class EnumBitMask
  {
    static_assert(enumCount<E>() <= 8, "EnumBitMask only supports Enums of size 8 or lower!");
    u8 m_Data;

  public:
    constexpr EnumBitMask()
      : EnumBitMask(0) {}
    constexpr EnumBitMask(E e)
      : EnumBitMask() { set(e); }
    constexpr EnumBitMask(u8 data)
      : m_Data(data) {}
    constexpr EnumBitMask(const std::initializer_list<E>& initializerList)
      : EnumBitMask()
    {
      for (E e : initializerList)
        set(e);
    }

    constexpr bool operator==(const EnumBitMask& other) const = default;

    constexpr EnumBitMask operator~() const { return EnumBitMask(~m_Data); }

    constexpr bool operator[](E e) const { return (m_Data >> enumIndex(e)) & 0x1; }

    constexpr bool empty() const { return m_Data == 0; }
    constexpr void set(E e) { m_Data |= u8Bit(enumIndex(e)); }

  private:
    EnumBitMask(std::underlying_type_t<E> underlyingData)
      : m_Data(underlyingData) {}
  };
}