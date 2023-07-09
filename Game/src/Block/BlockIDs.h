#pragma once
#include <cstdint>

using blockID = uint8_t;
using blockTexID = uint32_t;

namespace Block
{
  enum class Type : blockID
  {
    Air = 0,
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

  enum class Texture : blockTexID
  {
    GrassTop = 0,
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

    Null,
    Begin = 0, End = Null
  };
  using TextureIterator = Iterator<Texture, Texture::Begin, Texture::End>;
}