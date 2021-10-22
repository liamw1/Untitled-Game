#pragma once
#include <cstdint>

using blockID = uint8_t;

enum class BlockType : blockID
{
  Air = 0,
  Grass,
  Dirt
};