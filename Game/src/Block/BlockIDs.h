#pragma once
#include <cstdint>

using blockID = uint8_t;
using blockTexID = uint16_t;

enum class BlockType : blockID
{
  Air = 0,
  Grass,
  Dirt,
  Clay,
  Gravel,
  Sand,
  Stone,
  OakLog,
  OakLeaves
};

enum class BlockTexture : blockTexID
{
  GrassTop = 0,
  GrassSide,
  Dirt,
  Clay,
  Gravel,
  Sand,
  Stone,
  OakLogTop,
  OakLogSide,
  OakLeaves,

  Invisible,

  First = GrassTop, Last = OakLeaves
};
using BlockTextureIterator = Iterator<BlockTexture, BlockTexture::First, BlockTexture::Last>;