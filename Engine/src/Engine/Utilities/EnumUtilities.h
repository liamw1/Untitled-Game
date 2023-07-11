#pragma once

/*
  *:･ﾟ✧*:･ﾟ✧ ~~ Macro/Template Magic Zone ~~ ✧･ﾟ: *✧･ﾟ:*
*/

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



// ==================== Enabling Iteration for Enum Classes ==================== //
// Code borrowed from: https://stackoverflow.com/questions/261963/how-can-i-iterate-over-an-enum
template<typename Enum, Enum beginEnum, Enum endEnum>
class Iterator
{
  using val_t = typename std::underlying_type<Enum>::type;

public:
  Iterator(Enum valEnum)
    : m_Value(static_cast<val_t>(valEnum)) {}
  Iterator()
    : m_Value(static_cast<val_t>(beginEnum)) {}

  Iterator& operator++()
  {
    ++m_Value;
    return *this;
  }
  Enum operator*() { return static_cast<Enum>(m_Value); }
  bool operator!=(const Iterator& other) { return m_Value != other.m_Value; }

  Iterator begin() { return *this; }
  Iterator end() { return Iterator(endEnum); }
  Iterator next() const
  {
    Iterator copy = *this;
    return ++copy;
  }

private:
  val_t m_Value;
};