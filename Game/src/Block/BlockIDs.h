#pragma once
#include <cstdint>

using blockID = uint8_t;
using blockTexID = uint16_t;

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

    Null
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

    Null,
    Invisible,

    First = GrassTop, Last = OakLeaves
  };
  using TextureIterator = Iterator<Texture, Texture::First, Texture::Last>;
}