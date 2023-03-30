#pragma once
#include "BlockIDs.h"
#include "Util/CompoundType.h"

namespace Block
{
  using CompoundType = ::CompoundType<Type, 4, Block::Type::Null>;

  enum class Face : int
  {
    West, East, South, North, Bottom, Top,
    Null,

    Begin = 0, End = Null
  };
  using FaceIterator = Iterator<Face, Face::Begin, Face::End>;
  
  /*
    \returns The face directly opposite the given face.
  */
  constexpr Face operator!(const Face& face)
  {
    int faceID = static_cast<int>(face);
    Face oppFace = static_cast<Face>(faceID % 2 ? faceID - 1 : faceID + 1);
    return oppFace;
  }



  void Initialize();

  Texture GetTexture(Type block, Face face);
  std::string GetTexturePath(Texture texture);

  bool HasTransparency(Type block);
  bool HasCollision(Type block);

  bool IsTransparent(Texture texture);

  constexpr length_t Length() { return 0.5_m; }
  constexpr float LengthF() { return static_cast<float>(Length()); }
};

constexpr bool IsPositive(Block::Face face) { return static_cast<int>(face) % 2; }
constexpr int GetCoordID(Block::Face face) { return static_cast<int>(face) / 2; }