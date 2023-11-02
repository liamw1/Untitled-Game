#pragma once
#include "Engine/Core/Casting.h"
#include "Engine/Utilities/BitUtilities.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace eng::math
{
  enum class Axis
  {
    X, Y, Z,

    First = 0, Last = Z
  };
  using Axes = EnumIterator<Axis>;

  constexpr Axis Cycle(Axis axis)
  {
    return enumCastUnchecked<Axis>((toUnderlying(axis) + 1) % 3);
  }

  constexpr Axis GetMissing(Axis axisA, Axis axisB)
  {
    return enumCastUnchecked<Axis>(2 * (toUnderlying(axisA) + toUnderlying(axisB)) % 3);
  }



  enum class Direction
  {
    West, East, South, North, Bottom, Top,

    First = 0, Last = Top
  };
  using Directions = EnumIterator<Direction>;

  constexpr void operator++(Direction& direction)
  {
    direction = enumCastUnchecked<Direction>(toUnderlying(direction) + 1);
  }

  /*
    \returns The direction opposite the given direction.
  */
  constexpr Direction operator!(const Direction& direction)
  {
    std::underlying_type_t<Direction> directionID = toUnderlying(direction);
    Direction oppositeDirection = enumCastUnchecked<Direction>(directionID % 2 ? directionID - 1 : directionID + 1);
    return oppositeDirection;
  }

  constexpr bool IsUpstream(Direction direction) { return toUnderlying(direction) % 2; }
  constexpr Axis AxisOf(Direction direction) { return enumCastUnchecked<Axis>(toUnderlying(direction) / 2); }
  constexpr Direction ToDirection(Axis axis, bool isUpstream) { return enumCastUnchecked<Direction>(2 * toUnderlying(axis) + isUpstream); }

  class DirectionBitMask
  {
    u8 m_Data;

  public:
    constexpr DirectionBitMask()
      : m_Data(0) {}

    constexpr bool operator[](Direction direction) const { return (m_Data >> toUnderlying(direction)) & 0x1; }
    constexpr bool empty() const { return m_Data == 0; }
    constexpr void set(Direction direction) { m_Data |= u8Bit(toUnderlying(direction)); }
  };
}