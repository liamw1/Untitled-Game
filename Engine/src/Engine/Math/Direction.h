#pragma once
#include "Engine/Utilities/EnumUtilities.h"

enum class Axis : int
{
  X, Y, Z,

  Begin = 0, End = Z
};
using Axes = Engine::EnumIterator<Axis, Axis::Begin, Axis::End>;

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
constexpr Axis AxisOf(Direction direction) { return static_cast<Axis>(static_cast<int>(direction) / 2); }
constexpr Direction ToDirection(Axis axis, bool isUpstream) { return static_cast<Direction>(2 * static_cast<int>(axis) + isUpstream); }

template<typename T>
class DirectionArray
{
public:
  DirectionArray() = default;
  DirectionArray(const T& initialValue)
    : DirectionArray(initialValue, initialValue, initialValue, initialValue, initialValue, initialValue) {}
  DirectionArray(const T& west, const T& east, const T& south, const T& north, const T& bottom, const T& top)
    : DirectionArray(std::array<T, 6>{ west, east, south, north, bottom, top }) {}
  DirectionArray(const std::array<T, 6>& elements)
    : m_Data(elements) {}

  T& operator[](Direction direction)
  {
    return const_cast<T&>(static_cast<const DirectionArray*>(this)->operator[](direction));
  }
  const T& operator[](Direction direction) const
  {
    return m_Data[static_cast<int>(direction)];
  }

private:
  std::array<T, 6> m_Data;
};