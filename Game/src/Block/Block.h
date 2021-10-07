#pragma once
#include <cstdint>

static inline constexpr float s_BlockSize = 0.2f;

enum class BlockFace : uint8_t
{
  Top, Bottom, North, South, East, West,
  First = Top, Last = West
};

/*
  \returns The face directly opposite the given face.
*/
inline BlockFace operator!(const BlockFace& face)
{
  const uint8_t faceVal = static_cast<uint8_t>(face);
  BlockFace oppFace = static_cast<BlockFace>(faceVal % 2 == 0 ? faceVal + 1 : faceVal - 1);
  return oppFace;
}

class BlockFaceIterator
{
public:
  BlockFaceIterator(const BlockFace face)
    : value(static_cast<uint8_t>(face)) {}
  BlockFaceIterator()
    : value(static_cast<uint8_t>(BlockFace::First)) {}

  BlockFaceIterator& operator++()
  {
    ++value;
    return *this;
  }
  BlockFace operator*() { return static_cast<BlockFace>(value); }
  bool operator!=(const BlockFaceIterator& iter) { return value != iter.value; }

  BlockFaceIterator begin() { return *this; }
  BlockFaceIterator end()
  {
    static const BlockFaceIterator endIter = ++BlockFaceIterator(BlockFace::Last);
    return endIter;
  }

private:
  uint8_t value;
};