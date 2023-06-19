#include "GMpch.h"
#include "Block.h"
#include "Util/MultiDimArrays.h"
#include <Engine.h>

struct BlockUniforms
{
  const float blockLength = static_cast<float>(Block::Length());
};

static constexpr int c_BlockTypes = static_cast<int>(Block::Type::End) - static_cast<int>(Block::Type::Begin) + 1;  // Includes Null block
static constexpr int c_MaxBlockTypes = std::numeric_limits<blockID>::max() + 1;
static constexpr int c_MaxBlockTextures = 6 * c_MaxBlockTypes;
static constexpr int c_UniformBinding = 1;
static int s_BlocksInitialized = 0;
static bool s_Initialized = false;

static Array2D<Block::Texture, c_MaxBlockTypes, 6> s_TexIDs = AllocateArray2D<Block::Texture, c_MaxBlockTypes, 6>(Block::Texture::ErrorTexture);
static std::array<std::string, c_MaxBlockTextures> s_TexturePaths{};
static std::shared_ptr<Engine::TextureArray> s_TextureArray;
static const BlockUniforms s_BlockUniforms{};

static void assignTextures(Block::Type block, std::array<Block::Texture, 6> faceTextures)
{
  const blockID ID = static_cast<blockID>(block);
  for (Direction face : Directions())
  {
    int faceID = static_cast<int>(face);
    s_TexIDs(ID, faceID) = faceTextures[faceID];
  }
  s_BlocksInitialized++;
}

static void assignTextures(Block::Type block, Block::Texture topTexture, Block::Texture sideTextures, Block::Texture bottomTexture)
{
  assignTextures(block, { sideTextures, sideTextures, sideTextures, sideTextures, bottomTexture, topTexture });
}

static void assignTextures(Block::Type block, Block::Texture topBotTextures, Block::Texture sideTextures)
{
  assignTextures(block, { sideTextures, sideTextures, sideTextures, sideTextures, topBotTextures, topBotTextures });
}

static void assignTextures(Block::Type block, Block::Texture faceTextures)
{
  assignTextures(block, { faceTextures, faceTextures, faceTextures, faceTextures, faceTextures, faceTextures });
}

void Block::Initialize()
{
  Engine::UniformBuffer::Allocate(c_UniformBinding, sizeof(BlockUniforms));
  Engine::UniformBuffer::SetData(c_UniformBinding, &s_BlockUniforms);

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
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Invisible)] = "assets/textures/Invisible.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::ErrorTexture)] = "assets/textures/Checkerboard.png";

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
  assignTextures(Block::Type::Null, Block::Texture::ErrorTexture);

  s_TextureArray = Engine::TextureArray::Create(16, 128);
  for (Block::Texture texture : Block::TextureIterator())
  {
    blockTexID textureID = static_cast<blockTexID>(texture);
    if (s_TexturePaths[textureID] == "")
    {
      EN_ERROR("Block texture {0} has not been assign a path!", textureID);
      textureID = static_cast<blockTexID>(Block::Texture::ErrorTexture);
    }
    s_TextureArray->addTexture(s_TexturePaths[textureID]);
  }

  if (s_BlocksInitialized != c_BlockTypes)
    EN_ERROR("{0} of {1} blocks haven't been assigned textures!", c_BlockTypes - s_BlocksInitialized, c_BlockTypes);
  else
    s_Initialized = true;
}

std::shared_ptr<Engine::TextureArray> Block::GetTextureArray()
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  return s_TextureArray;
}

Block::Texture Block::GetTexture(Type block, Direction face)
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  return s_TexIDs(static_cast<blockID>(block), static_cast<int>(face));
}

bool Block::HasTransparency(Type block)
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  switch (block)
  {
    case Block::Type::Air:        return true;
    case Block::Type::OakLeaves:  return true;
    default:                      return false;
  }
}

bool Block::HasCollision(Type block)
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  switch (block)
  {
    case Block::Type::Air:  return false;
    default:                return true;
  }
}

bool Block::IsTransparent(Texture texture)
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  switch (texture)
  {
    case Block::Texture::Invisible: return true;
    case Block::Texture::OakLeaves: return true;
    default:                        return false;
  }
}
