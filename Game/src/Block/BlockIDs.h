#pragma once
#include <cstdint>

using blockID = uint8_t;

enum class Block : uint8_t
{
  Air = 0,
  Grass,
  Sand
};