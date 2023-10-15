#pragma once
#include <Engine.h>

namespace block
{
  enum class ID : uint8_t
  {
    Air,
    Grass,
    Dirt,
    Clay,
    Gravel,
    Sand,
    Snow,
    Stone,
    OakLog,
    OakLeaves,
    FallLeaves,
    Glass,
    Water,

    Null,
    Begin = 0, End = Null
  };
  using IDs = eng::EnumIterator<ID>;

  enum class TextureID : uint32_t
  {
    GrassTop,
    GrassSide,
    Dirt,
    Clay,
    Gravel,
    Sand,
    Snow,
    Stone,
    OakLogTop,
    OakLogSide,
    OakLeaves,
    FallLeaves,
    Glass,
    Water,

    Invisible,
    ErrorTexture,

    Begin = 0, End = ErrorTexture
  };
  using Textures = eng::EnumIterator<TextureID>;
}