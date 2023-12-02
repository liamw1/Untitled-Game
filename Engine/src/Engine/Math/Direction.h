#pragma once
#include "Engine/Core/Casting.h"
#include "Engine/Utilities/EnumUtilities.h"

namespace eng::math
{
  enum class Axis
  {
    X, Y, Z,

    First = 0, Last = Z
  };
  using Axes = EnumIterator<Axis>;

  constexpr Axis cycle(Axis axis)
  {
    return enumCastUnchecked<Axis>((enumIndex(axis) + 1) % 3);
  }

  constexpr Axis getMissing(Axis axisA, Axis axisB)
  {
    return enumCastUnchecked<Axis>(2 * (enumIndex(axisA) + enumIndex(axisB)) % 3);
  }



  enum class Direction
  {
    West, East, South, North, Bottom, Top,

    First = 0, Last = Top
  };
  using Directions = EnumIterator<Direction>;

  constexpr void operator++(Direction& direction)
  {
    direction = enumCastUnchecked<Direction>(enumIndex(direction) + 1);
  }

  /*
    \returns The direction opposite the given direction.
  */
  constexpr Direction operator!(const Direction& direction)
  {
    std::underlying_type_t<Direction> directionID = enumIndex(direction);
    Direction oppositeDirection = enumCastUnchecked<Direction>(directionID % 2 ? directionID - 1 : directionID + 1);
    return oppositeDirection;
  }

  constexpr bool isUpstream(Direction direction) { return enumIndex(direction) % 2; }
  constexpr Axis axisOf(Direction direction) { return enumCastUnchecked<Axis>(enumIndex(direction) / 2); }
  constexpr Direction toDirection(Axis axis, bool isUpstream) { return enumCastUnchecked<Direction>(2 * enumIndex(axis) + isUpstream); }
}