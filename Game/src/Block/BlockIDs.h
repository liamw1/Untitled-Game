#pragma once
#include <Engine.h>

namespace block
{
  enum class ID : u8
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
    First = 0, Last = Null
  };
  using IDs = eng::EnumIterator<ID>;

  enum class TextureID : u32
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

    First = 0, Last = ErrorTexture
  };
  using Textures = eng::EnumIterator<TextureID>;
}