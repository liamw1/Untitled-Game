#include "GMpch.h"
#include "Block.h"

namespace block
{
  struct BlockUniformData
  {
    const float blockLength = static_cast<float>(length());
  };
  
  static constexpr int c_UniformBinding = 1;
  static int s_BlocksInitialized = 0;
  static bool s_Initialized = false;

  static eng::EnumArray<bool, block::ID> s_HasTransparency;
  static eng::EnumArray<bool, block::ID> s_HasCollision;
  static eng::EnumArray<eng::math::Float4, block::TextureID> s_TextureAverageColors;
  static eng::EnumArray<eng::math::DirectionArray<TextureID>, block::ID> s_TextureIDs;
  static eng::EnumArray<std::filesystem::path, block::TextureID> s_TexturePaths;
  static std::shared_ptr<eng::TextureArray> s_TextureArray;
  static std::unique_ptr<eng::Uniform> s_Uniform;
  static const BlockUniformData s_BlockUniformData{};
  
  static void assignTextures(ID block, const eng::math::DirectionArray<TextureID>& faceTextures)
  {
    s_TextureIDs[block] = faceTextures;
    s_BlocksInitialized++;
  }
  
  static void assignTextures(ID block, TextureID topTexture, TextureID sideTextures, TextureID bottomTexture)
  {
    assignTextures(block, { sideTextures, sideTextures, sideTextures, sideTextures, bottomTexture, topTexture });
  }
  
  static void assignTextures(ID block, TextureID topBotTextures, TextureID sideTextures)
  {
    assignTextures(block, { sideTextures, sideTextures, sideTextures, sideTextures, topBotTextures, topBotTextures });
  }
  
  static void assignTextures(ID block, TextureID faceTextures)
  {
    assignTextures(block, { faceTextures, faceTextures, faceTextures, faceTextures, faceTextures, faceTextures });
  }

  static void assignTextures()
  {
    std::filesystem::path textureFolder = "assets/textures";
    std::filesystem::path tileFolder = textureFolder / "voxel-pack/PNG/Tiles";

    s_TexturePaths[TextureID::GrassTop] = tileFolder / "grass_top.png";
    s_TexturePaths[TextureID::GrassSide] = tileFolder / "dirt_grass.png";
    s_TexturePaths[TextureID::Dirt] = tileFolder / "dirt.png";
    s_TexturePaths[TextureID::Clay] = tileFolder / "greysand.png";
    s_TexturePaths[TextureID::Gravel] = tileFolder / "gravel_stone.png";
    s_TexturePaths[TextureID::Sand] = tileFolder / "sand.png";
    s_TexturePaths[TextureID::Snow] = tileFolder / "snow.png";
    s_TexturePaths[TextureID::Stone] = tileFolder / "stone.png";
    s_TexturePaths[TextureID::OakLogTop] = tileFolder / "trunk_top.png";
    s_TexturePaths[TextureID::OakLogSide] = tileFolder / "trunk_side.png";
    s_TexturePaths[TextureID::OakLeaves] = tileFolder / "leaves_transparent.png";
    s_TexturePaths[TextureID::FallLeaves] = tileFolder / "leaves_orange_transparent.png";
    s_TexturePaths[TextureID::Glass] = tileFolder / "glass.png";
    s_TexturePaths[TextureID::Water] = tileFolder / "water_transparent.png";
    s_TexturePaths[TextureID::Invisible] = textureFolder / "Invisible.png";
    s_TexturePaths[TextureID::ErrorTexture] = textureFolder / "Checkerboard.png";

    assignTextures(ID::Air, TextureID::Invisible);
    assignTextures(ID::Grass, TextureID::GrassTop, TextureID::GrassSide, TextureID::Dirt);
    assignTextures(ID::Dirt, TextureID::Dirt);
    assignTextures(ID::Clay, TextureID::Clay);
    assignTextures(ID::Gravel, TextureID::Gravel);
    assignTextures(ID::Sand, TextureID::Sand);
    assignTextures(ID::Snow, TextureID::Snow);
    assignTextures(ID::Stone, TextureID::Stone);
    assignTextures(ID::OakLog, TextureID::OakLogTop, TextureID::OakLogSide);
    assignTextures(ID::OakLeaves, TextureID::OakLeaves);
    assignTextures(ID::FallLeaves, TextureID::FallLeaves);
    assignTextures(ID::Glass, TextureID::Glass);
    assignTextures(ID::Water, TextureID::Water);
    assignTextures(ID::Null, TextureID::ErrorTexture);

    s_TextureArray = eng::TextureArray::Create(16, 128);
    for (TextureID texture : Textures())
    {
      if (s_TexturePaths[texture] == "")
      {
        EN_ERROR("Block texture {0} has not been assign a path!", std::underlying_type_t<block::TextureID>(texture));
        texture = TextureID::ErrorTexture;
      }

      eng::Image textureImage(s_TexturePaths[texture]);
      s_TextureArray->addTexture(textureImage);

      eng::math::Float4 textureAverageColor = textureImage.averageColor();
      textureAverageColor.a = 1.0f;
      s_TextureAverageColors[texture] = textureAverageColor;
    }

    static constexpr int numBlockTypes = eng::enumRange<block::ID>();
    if (s_BlocksInitialized != numBlockTypes)
      EN_ERROR("{0} of {1} blocks haven't been assigned textures!", numBlockTypes - s_BlocksInitialized, numBlockTypes);
  }

  static void assignTransparency()
  {
    for (ID blockID : IDs())
      switch (blockID)
      {
        case ID::Air:
        case ID::OakLeaves:
        case ID::FallLeaves:
        case ID::Glass:
        case ID::Water:
          s_HasTransparency[blockID] = true;
          break;
        default: s_HasTransparency[blockID] = false;
      }
  }

  static void assignCollisionality()
  {
    for (ID blockID : IDs())
      switch (blockID)
      {
        case ID::Air:
        case ID::Water:
          s_HasCollision[blockID] = false;
          break;
        default: s_HasCollision[blockID] = true;
      }
  }



  void initialize()
  {
    s_Uniform = eng::Uniform::Create(c_UniformBinding, sizeof(BlockUniformData));
    s_Uniform->set(&s_BlockUniformData, sizeof(BlockUniformData));

    assignTextures();
    assignTransparency();
    assignCollisionality();

    s_Initialized = true;
  }

  std::shared_ptr<eng::TextureArray> getTextureArray()
  {
    EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
    return s_TextureArray;
  }



  TextureID Type::texture(eng::math::Direction face) const
  {
    EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
    return s_TextureIDs[m_TypeID][face];
  }

  bool Type::hasTransparency() const
  {
    EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
    return s_HasTransparency[m_TypeID];
  }

  bool Type::hasCollision() const
  {
    EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
    return s_HasCollision[m_TypeID];
  }



  Light::Light()
    : Light(0) {}
  Light::Light(int8_t sunlight)
    : m_Sunlight(sunlight)
  {
    EN_ASSERT(eng::debug::BoundsCheck(sunlight, 0, MaxValue() + 1), "Invalid value for sunlight!");
  }

  bool Light::operator==(Light other) const
  {
    return m_Sunlight == other.m_Sunlight;
  }

  int8_t Light::sunlight() const
  {
    return m_Sunlight;
  }
}