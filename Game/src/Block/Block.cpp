#include "GMpch.h"
#include "Block.h"

struct BlockUniformData
{
  const float blockLength = static_cast<float>(Block::Length());
};

static constexpr int c_BlockTypes = static_cast<int>(Block::Type::End) - static_cast<int>(Block::Type::Begin) + 1;  // Includes Null block
static constexpr int c_MaxBlockTypes = std::numeric_limits<blockID>::max() + 1;
static constexpr int c_MaxBlockTextures = 6 * c_MaxBlockTypes;
static constexpr int c_UniformBinding = 1;
static int s_BlocksInitialized = 0;
static bool s_Initialized = false;

static ArrayRect<Block::Texture, int, 0, c_MaxBlockTypes, 0, 6> s_TexIDs(Block::Texture::ErrorTexture);
static std::array<std::filesystem::path, c_MaxBlockTextures> s_TexturePaths{};
static std::shared_ptr<Engine::TextureArray> s_TextureArray;
static std::unique_ptr<Engine::Uniform> s_Uniform;
static const BlockUniformData s_BlockUniformData{};

static void assignTextures(Block::Type block, std::array<Block::Texture, 6> faceTextures)
{
  const blockID ID = static_cast<blockID>(block);
  for (Direction face : Directions())
  {
    int faceID = static_cast<int>(face);
    s_TexIDs[ID][faceID] = faceTextures[faceID];
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
  s_Uniform = Engine::Uniform::Create(c_UniformBinding, sizeof(BlockUniformData));
  s_Uniform->set(&s_BlockUniformData, sizeof(BlockUniformData));

  std::filesystem::path textureFolder = "assets/textures";
  std::filesystem::path tileFolder = textureFolder / "voxel-pack/PNG/Tiles";

  s_TexturePaths[static_cast<blockTexID>(Block::Texture::GrassTop)] = tileFolder / "grass_top.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::GrassSide)] = tileFolder / "dirt_grass.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Dirt)] = tileFolder / "dirt.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Clay)] = tileFolder / "greysand.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Gravel)] = tileFolder / "gravel_stone.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Sand)] = tileFolder / "sand.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Snow)] = tileFolder / "snow.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Stone)] = tileFolder / "stone.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::OakLogTop)] = tileFolder / "trunk_top.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::OakLogSide)] = tileFolder / "trunk_side.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::OakLeaves)] = tileFolder / "leaves_transparent.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::FallLeaves)] = tileFolder / "leaves_orange_transparent.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Glass)] = tileFolder / "glass.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Water)] = tileFolder / "water_transparent.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::Invisible)] = textureFolder / "Invisible.png";
  s_TexturePaths[static_cast<blockTexID>(Block::Texture::ErrorTexture)] = textureFolder / "Checkerboard.png";

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
  assignTextures(Block::Type::FallLeaves, Block::Texture::FallLeaves);
  assignTextures(Block::Type::Glass, Block::Texture::Glass);
  assignTextures(Block::Type::Water, Block::Texture::Water);
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
    s_TextureArray->addTexture(s_TexturePaths[textureID].string());
  }

  if (s_BlocksInitialized != c_BlockTypes)
    EN_ERROR("{0} of {1} blocks haven't been assigned textures!", c_BlockTypes - s_BlocksInitialized, c_BlockTypes);

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
  return s_TexIDs[static_cast<blockID>(block)][static_cast<int>(face)];
}

bool Block::HasTransparency(Texture texture)
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  switch (texture)
  {
    case Texture::Invisible:   return true;
    case Texture::OakLeaves:   return true;
    case Texture::FallLeaves:  return true;
    case Texture::Glass:       return true;
    case Texture::Water:       return true;
    default:                   return false;
  }
}

bool Block::HasTransparency(Type block)
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  switch (block)
  {
    case Type::Air:        return true;
    case Type::OakLeaves:  return true;
    case Type::FallLeaves: return true;
    case Type::Glass:      return true;
    case Type::Water:      return true;
    default:               return false;
  }
}

bool Block::HasCollision(Type block)
{
  EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
  switch (block)
  {
    case Type::Air:    return false;
    case Type::Water:  return false;
    default:           return true;
  }
}



Block::Light::Light()
  : Light(0) {}
Block::Light::Light(int8_t sunlight)
  : m_Sunlight(sunlight)
{
  EN_ASSERT(Engine::Debug::BoundsCheck(sunlight, 0, MaxValue() + 1), "Invalid value for sunlight!");
}

bool Block::Light::operator==(Light other) const
{
  return m_Sunlight == other.m_Sunlight;
}

int8_t Block::Light::sunlight() const
{
  return m_Sunlight;
}