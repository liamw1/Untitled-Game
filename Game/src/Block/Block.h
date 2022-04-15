#pragma once
#include "BlockIDs.h"

namespace Block
{
  enum class Face : int
  {
    West, East, South, North, Bottom, Top,
    First = West, Last = Top
  };
  using FaceIterator = Iterator<Face, Face::First, Face::Last>;
  
  /*
    \returns The face directly opposite the given face.
  */
  constexpr Face operator!(const Face& face)
  {
    const int faceID = static_cast<int>(face);
    Face oppFace = static_cast<Face>(faceID % 2 ? faceID - 1 : faceID + 1);
    return oppFace;
  }

  void Initialize();

  Texture GetTexture(Type block, Face face);
  std::string GetTexturePath(Texture texture);

  bool HasTransparency(Type block);
  bool HasCollision(Type block);

  bool IsTransparent(Texture texture);

  constexpr length_t Length() { return 0.5; }
};

constexpr bool IsPositive(Block::Face face) { return static_cast<int>(face) % 2; }
constexpr int GetCoordID(Block::Face face) { return static_cast<int>(face) / 2; }