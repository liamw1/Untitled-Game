#pragma once
#include <cstdint>

static inline constexpr float s_BlockSize = 0.2f;

enum class BlockFace : uint8_t
{
  Top, Bottom, North, South, East, West
};