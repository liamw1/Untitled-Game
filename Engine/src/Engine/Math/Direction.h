#pragma once
#include "Engine/Utilities/BitUtilities.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace eng::math
{
  enum class Axis : int
  {
    X, Y, Z,

    Begin = 0, End = Z
  };
  using Axes = EnumIterator<Axis>;

  constexpr Axis Cycle(Axis axis)
  {
    int axisID = static_cast<int>(axis);
    return static_cast<Axis>((axisID + 1) % 3);
  }

  constexpr Axis GetMissing(Axis axisA, Axis axisB)
  {
    int u = static_cast<int>(axisA);
    int v = static_cast<int>(axisB);
    int w = (2 * (u + v)) % 3;
    return static_cast<Axis>(w);
  }



  enum class Direction : int
  {
    West, East, South, North, Bottom, Top,

    Begin = 0, End = Top
  };
  using Directions = EnumIterator<Direction>;

  template<typename T>
  using DirectionArray = EnumArray<T, Direction>;

  /*
    \returns The next direction in sequence.
  */
  constexpr void operator++(Direction& direction)
  {
    int directionID = static_cast<int>(direction);
    direction = static_cast<Direction>(directionID + 1);
  }

  /*
    \returns The direction opposite the given direction.
  */
  constexpr Direction operator!(const Direction& direction)
  {
    int directionID = static_cast<int>(direction);
    Direction oppositeDirection = static_cast<Direction>(directionID % 2 ? directionID - 1 : directionID + 1);
    return oppositeDirection;
  }

  constexpr bool IsUpstream(Direction direction) { return static_cast<int>(direction) % 2; }
  constexpr Axis AxisOf(Direction direction) { return static_cast<Axis>(static_cast<int>(direction) / 2); }
  constexpr Direction ToDirection(Axis axis, bool isUpstream) { return static_cast<Direction>(2 * static_cast<int>(axis) + isUpstream); }

  class DirectionBitMask
  {
  public:
    constexpr DirectionBitMask()
      : m_Data(0) {}

    constexpr bool operator[](Direction direction) const { return (m_Data >> static_cast<int>(direction)) & 0x1; }
    constexpr bool empty() const { return m_Data == 0; }
    constexpr void set(Direction direction) { m_Data |= u8Bit(static_cast<int>(direction)); }

  private:
    uint8_t m_Data;
  };
}