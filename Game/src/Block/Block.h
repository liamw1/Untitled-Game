#pragma once
#include "BlockIDs.h"

enum class BlockFace : int
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
  const int faceID = static_cast<int>(face);
  BlockFace oppFace = static_cast<BlockFace>(faceID % 2 == 0 ? faceID + 1 : faceID - 1);
  return oppFace;
}

class Block
{
public:
  static void Initialize();

  static BlockTexture GetTexture(BlockType block, BlockFace face);
  static std::string GetTexturePath(BlockTexture texture);

  static bool HasTransparency(BlockType block);
  static bool HasCollision(BlockType block);

  static constexpr length_t Length() { return s_BlockLength; }

private:
  static constexpr length_t s_BlockLength = static_cast<length_t>(0.5);
  static constexpr int s_MaxBlockTypes = std::numeric_limits<blockID>::max() + 1;
  static constexpr int s_MaxBlockTextures = 6 * s_MaxBlockTypes;
  static bool s_Initialized;

  static std::array<std::array<BlockTexture, 6>, s_MaxBlockTypes> s_TexIDs;
  static std::array<std::string, s_MaxBlockTextures> s_TexturePaths;

  static void assignTextures(BlockType block, BlockTexture faceTextures);
  static void assignTextures(BlockType block, BlockTexture topBotTextures, BlockTexture sideTextures);
  static void assignTextures(BlockType block, BlockTexture topTexture, BlockTexture sideTextures, BlockTexture bottomTexture);
};