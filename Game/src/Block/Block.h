#pragma once
#include <cstdint>

enum class BlockFace : uint8_t
{
  East, West, North, South, Top, Bottom,
  First = East, Last = Bottom
};
using BlockFaceIterator = Iterator<BlockFace, BlockFace::First, BlockFace::Last>;

/*
  \returns The face directly opposite the given face.
*/
inline BlockFace operator!(const BlockFace& face)
{
  const uint8_t faceVal = static_cast<uint8_t>(face);
  BlockFace oppFace = static_cast<BlockFace>(faceVal % 2 == 0 ? faceVal + 1 : faceVal - 1);
  return oppFace;
}

class Block
{
public:
  static constexpr float Length() { return s_BlockSize; }

private:
  static constexpr float s_BlockSize = 0.2f;
};