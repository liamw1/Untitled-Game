#pragma once
#include "Engine/Utilities/EnumUtilities.h"

enum class Direction : int
{
  West, East, South, North, Bottom, Top,
  Null,

  Begin = 0, End = Null
};
using Directions = Engine::EnumIterator<Direction, Direction::Begin, Direction::End>;

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
constexpr int GetCoordID(Direction direction) { return static_cast<int>(direction) / 2; }
constexpr Direction FromCoordID(int coordID, bool isUpstream) { return static_cast<Direction>(2 * coordID + isUpstream); }
