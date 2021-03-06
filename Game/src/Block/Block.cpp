#include "GMpch.h"
#include "Block.h"
#include "Util/MultiDimArrays.h"
#include <Engine.h>

struct BlockUniforms
{
  const float blockLength = static_cast<float>(Block::Length());
};

static constexpr int s_MaxBlockTypes = std::numeric_limits<blockID>::max() + 1;
static constexpr int s_MaxBlockTextures = 6 * s_MaxBlockTypes;
static constexpr int s_UniformBinding = 1;
static bool s_Initialized = false;

static StackArray2D<Block::Texture, s_MaxBlockTypes, 6> s_TexIDs{};
static std::array<std::string, s_MaxBlockTextures> s_TexturePaths{};
static const BlockUniforms s_BlockUniforms{};

static void assignTextures(Block::Type block, Block::Texture topTexture, Block::Texture sideTextures, Block::Texture bottomTexture)
{
  const blockID ID = static_cast<blockID>(block);

  for (Block::Face face : Block::FaceIterator())
  {
    const int faceID = static_cast<int>(face);
    switch (face)
    {
      case Block::Face::Top:    s_TexIDs[ID][faceID] = topTexture;      break;
      case Block::Face::Bottom: s_TexIDs[ID][faceID] = bottomTexture;   break;
      default:                s_TexIDs[ID][faceID] = sideTextures;
    }
  }
}

static void assignTextures(Block::Type block, Block::Texture faceTextures)
{
  for (Block::Face face : Block::FaceIterator())
    s_TexIDs[static_cast<blockID>(block)][static_cast<int>(face)] = faceTextures;
}

static void assignTextures(Block::Type block, Block::Texture topBotTextures, Block::Texture sideTextures)
{
  assignTextures(block, topBotTextures, sideTextures, topBotTextures);
}

void Block::Initialize()
{
  Engine::UniformBuffer::Allocate(s_UniformBinding, sizeof(BlockUniforms));
  Engine::UniformBuffer::SetData(s_UniformBinding, &s_BlockUniforms);

  s_TexturePaths[static_cast<blockTexID>(Block::Texture::GrassTop)] = "assets/textures/voxel-pack/PNG/Tiles/grass_top.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::GrassSide)] = "assets/textures/voxel-pack/PNG/Tiles/grass_top.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Dirt)] = "assets/textures/voxel-pack/PNG/Tiles/dirt.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Clay)] = "assets/textures/voxel-pack/PNG/Tiles/greysand.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Gravel)] = "assets/textures/voxel-pack/PNG/Tiles/gravel_stone.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Sand)] = "assets/textures/voxel-pack/PNG/Tiles/sand.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Snow)] = "assets/textures/voxel-pack/PNG/Tiles/snow.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Stone)] = "assets/textures/voxel-pack/PNG/Tiles/Stone.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::OakLogTop)] = "assets/textures/voxel-pack/PNG/Tiles/trunk_top.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::OakLogSide)] = "assets/textures/voxel-pack/PNG/Tiles/trunk_side.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::OakLeaves)] = "assets/textures/voxel-pack/PNG/Tiles/leaves_transparent.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Null)] = "assets/textures/Checkerboard.png";

  assignTextures(Block::Type::Air, Block::Texture::Invisible);
  assignTextures(Block::Type::Grass, Block::Texture::GrassTop, Block::Texture::GrassSide, Block::Texture::Dirt);
  assignTextures(Block::Type::Dirt, Block::Texture::Dirt);
  assignTextures(Block::Type::Clay, Block::Texture::Clay);
  assignTextures(Block::Type::Gravel, Block::Texture::Gravel);
  assignTextures(Block::Type::Sand, Block::Texture::Sand);
  assignTextures(Block::Type::Snow, Block::Texture::Snow);
  assignTextures(Block::Type::Stone, Block::Texture::Stone);
  assignTextures(Block::Type::OakLog, Block::Texture::OakLogTop, Block::Texture::OakLogSide);
  assignTextures(Block::Type::OakLeaves, Block::Texture::OakLeaves);
  assignTextures(Block::Type::Null, Block::Texture::Null);

  s_Initialized = true;
}

Block::Texture Block::GetTexture(Type block, Face face)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  return s_TexIDs[static_cast<blockID>(block)][static_cast<int>(face)];
}

std::string Block::GetTexturePath(Texture texture)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  return s_TexturePaths[static_cast<blockTexID>(texture)];
}

bool Block::HasTransparency(Type block)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  switch (block)
  {
    case Block::Type::Air:        return true;
    case Block::Type::OakLeaves:  return true;
    default:                      return false;
  }
}

bool Block::HasCollision(Type block)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  switch (block)
  {
    case Block::Type::Air:  return false;
    default:                return true;
  }
}

bool Block::IsTransparent(Texture texture)
{
  EN_ASSERT(s_Initialized, "Block class has not been initialized!");
  switch (texture)
  {
    case Block::Texture::Invisible: return true;
    case Block::Texture::OakLeaves: return true;
    default:                        return false;
  }
}
