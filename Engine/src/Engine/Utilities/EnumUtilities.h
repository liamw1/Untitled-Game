#pragma once

// ==================== Enabling Bitmasking for Enum Classes ==================== //
namespace Engine
{
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



  // ==================== Enabling Iteration for Enum Classes ==================== //
  // Code borrowed from: https://stackoverflow.com/questions/261963/how-can-i-iterate-over-an-enum
  template<typename Enum, Enum beginEnum, Enum endEnum>
  class EnumIterator
  {
    using val_t = typename std::underlying_type<Enum>::type;

  public:
    EnumIterator(Enum valEnum)
      : m_Value(static_cast<val_t>(valEnum)) {}
    EnumIterator()
      : m_Value(static_cast<val_t>(beginEnum)){}

    EnumIterator& operator++()
    {
      ++m_Value;
      return *this;
    }
    Enum operator*() { return static_cast<Enum>(m_Value); }
    bool operator!=(const EnumIterator& other) { return m_Value != other.m_Value; }

    EnumIterator begin() { return *this; }
    EnumIterator end() { return EnumIterator(endEnum); }
    EnumIterator next() const
    {
      EnumIterator copy = *this;
      return ++copy;
    }

  private:
    val_t m_Value;
  };
}