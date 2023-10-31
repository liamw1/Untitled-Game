#pragma once
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
    i32 axisID = static_cast<i32>(axis);
    return static_cast<Axis>((axisID + 1) % 3);
  }

  constexpr Axis GetMissing(Axis axisA, Axis axisB)
  {
    i32 u = static_cast<i32>(axisA);
    i32 v = static_cast<i32>(axisB);
    i32 w = (2 * (u + v)) % 3;
    return static_cast<Axis>(w);
  }



  enum class Direction
  {
    West, East, South, North, Bottom, Top,

    First = 0, Last = Top
  };
  using Directions = EnumIterator<Direction>;

  /*
    \returns The next direction in sequence.
  */
  constexpr void operator++(Direction& direction)
  {
    i32 directionID = static_cast<i32>(direction);
    direction = static_cast<Direction>(directionID + 1);
  }

  /*
    \returns The direction opposite the given direction.
  */
  constexpr Direction operator!(const Direction& direction)
  {
    i32 directionID = static_cast<i32>(direction);
    Direction oppositeDirection = static_cast<Direction>(directionID % 2 ? directionID - 1 : directionID + 1);
    return oppositeDirection;
  }

  constexpr bool IsUpstream(Direction direction) { return static_cast<i32>(direction) % 2; }
  constexpr Axis AxisOf(Direction direction) { return static_cast<Axis>(static_cast<i32>(direction) / 2); }
  constexpr Direction ToDirection(Axis axis, bool isUpstream) { return static_cast<Direction>(2 * static_cast<i32>(axis) + isUpstream); }

  class DirectionBitMask
  {
  public:
    constexpr DirectionBitMask()
      : m_Data(0) {}

    constexpr bool operator[](Direction direction) const { return (m_Data >> static_cast<i32>(direction)) & 0x1; }
    constexpr bool empty() const { return m_Data == 0; }
    constexpr void set(Direction direction) { m_Data |= u8Bit(static_cast<i32>(direction)); }

  private:
    u8 m_Data;
  };
}