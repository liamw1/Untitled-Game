#include "GMpch.h"
#include "Block.h"

StackArray2D<BlockTexture, Block::s_MaxBlockTypes, 6> Block::s_TexIDs{};
std::array<std::string, Block::s_MaxBlockTextures> Block::s_TexturePaths{};

bool Block::s_Initialized = false;

void Block::Initialize()
{
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::GrassTop)] = "assets/textures/voxel-pack/PNG/Tiles/grass_top.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::GrassSide)] = "assets/textures/voxel-pack/PNG/Tiles/grass_top.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::Dirt)] = "assets/textures/voxel-pack/PNG/Tiles/dirt.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::Clay)] = "assets/textures/voxel-pack/PNG/Tiles/greysand.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::Gravel)] = "assets/textures/voxel-pack/PNG/Tiles/gravel_stone.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::Sand)] = "assets/textures/voxel-pack/PNG/Tiles/sand.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::Snow)] = "assets/textures/voxel-pack/PNG/Tiles/snow.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::Stone)] = "assets/textures/voxel-pack/PNG/Tiles/Stone.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::OakLogTop)] = "assets/textures/voxel-pack/PNG/Tiles/trunk_top.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::OakLogSide)] = "assets/textures/voxel-pack/PNG/Tiles/trunk_side.png";
  s_TexturePaths[static_cast<blockTexID>(BlockTexture::OakLeaves)] = "assets/textures/voxel-pack/PNG/Tiles/leaves_transparent.png";

  assignTextures(BlockType::Air, BlockTexture::Invisible);
  assignTextures(BlockType::Grass, BlockTexture::GrassTop, BlockTexture::GrassSide, BlockTexture::Dirt);
  assignTextures(BlockType::Dirt, BlockTexture::Dirt);
  assignTextures(BlockType::Clay, BlockTexture::Clay);
  assignTextures(BlockType::Gravel, BlockTexture::Gravel);
  assignTextures(BlockType::Sand, BlockTexture::Sand);
  assignTextures(BlockType::Snow, BlockTexture::Snow);
  assignTextures(BlockType::Stone, BlockTexture::Stone);
  assignTextures(BlockType::OakLog, BlockTexture::OakLogTop, BlockTexture::OakLogSide);
  assignTextures(BlockType::OakLeaves, BlockTexture::OakLeaves);

  s_Initialized = true;
}

BlockTexture Block::GetTexture(BlockType block, BlockFace face)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  return s_TexIDs[static_cast<blockID>(block)][static_cast<int>(face)];
}

std::string Block::GetTexturePath(BlockTexture texture)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  return s_TexturePaths[static_cast<blockTexID>(texture)];
}

bool Block::HasTransparency(BlockType block)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  return block == BlockType::Air || block == BlockType::OakLeaves;
}

bool Block::HasCollision(BlockType block)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  return block != BlockType::Air;
}

void Block::assignTextures(BlockType block, BlockTexture faceTextures)
{
  for (BlockFace face : BlockFaceIterator())
    s_TexIDs[static_cast<blockID>(block)][static_cast<int>(face)] = faceTextures;
}

void Block::assignTextures(BlockType block, BlockTexture topBotTextures, BlockTexture sideTextures)
{
  assignTextures(block, topBotTextures, sideTextures, topBotTextures);
}

void Block::assignTextures(BlockType block, BlockTexture topTexture, BlockTexture sideTextures, BlockTexture bottomTexture)
{
  const blockID ID = static_cast<blockID>(block);

  for (BlockFace face : BlockFaceIterator())
  {
    const int faceID = static_cast<int>(face);
    switch (face)
    {
      case BlockFace::Top:    s_TexIDs[ID][faceID] = topTexture;      break;
      case BlockFace::Bottom: s_TexIDs[ID][faceID] = bottomTexture;   break;
      default:                s_TexIDs[ID][faceID] = sideTextures;
    }
  }
}
