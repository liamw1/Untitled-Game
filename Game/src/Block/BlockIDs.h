#pragma once
#include <cstdint>

using blockID = uint8_t;

enum class Block : uint8_t
{
  Air = 0,
  Grass,
  Sand
};



enum BlockFace : uint8_t
{
  Top, Bottom, North, South, East, West, End
};