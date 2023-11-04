#include "GMpch.h"
#include "Block.h"

namespace block
{
  static eng::EnumArray<bool, ID> computeTransparencies()
  {
    eng::EnumArray<bool, ID> transparencies{};
    for (ID blockID : IDs())
    {
      switch (blockID)
      {
        case ID::Air:
        case ID::OakLeaves:
        case ID::FallLeaves:
        case ID::Glass:
        case ID::Water:
          transparencies[blockID] = true;
          break;
        default: transparencies[blockID] = false;
      }
    }
    return transparencies;
  }

  static eng::EnumArray<bool, ID> computeCollisionalities()
  {
    eng::EnumArray<bool, ID> collisionalities{};
    for (ID blockID : IDs())
    {
      switch (blockID)
      {
        case ID::Air:
        case ID::Water:
          collisionalities[blockID] = false;
          break;
        default: collisionalities[blockID] = true;
      }
    }
    return collisionalities;
  }

  static eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID westTexture,   TextureID eastTexture,
                                                                             TextureID southTexture,  TextureID northTexture,
                                                                             TextureID bottomTexture, TextureID topTexture)
  {
    return { { eng::math::Direction::West,   westTexture   }, { eng::math::Direction::East,  eastTexture  },
             { eng::math::Direction::South,  southTexture  }, { eng::math::Direction::North, northTexture },
             { eng::math::Direction::Bottom, bottomTexture }, { eng::math::Direction::Top,   topTexture   } };
  }

  static eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID topTexture, TextureID sideTextures, TextureID bottomTexture)
  {
    return createBlockTextures(sideTextures, sideTextures, sideTextures, sideTextures, bottomTexture, topTexture);
  }

  static eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID topBotTextures, TextureID sideTextures)
  {
    return createBlockTextures(topBotTextures, sideTextures, topBotTextures);
  }

  static eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID faceTextures)
  {
    return createBlockTextures(faceTextures, faceTextures);
  }

  static eng::EnumArray<eng::EnumArray<TextureID, eng::math::Direction>, ID> computeTextureIDs()
  {
    eng::EnumArray<eng::EnumArray<TextureID, eng::math::Direction>, ID> textureIDs;
    textureIDs[ID::Air] = createBlockTextures(TextureID::Invisible);
    textureIDs[ID::Grass] = createBlockTextures(TextureID::GrassTop, TextureID::GrassSide, TextureID::Dirt);
    textureIDs[ID::Dirt] = createBlockTextures(TextureID::Dirt);
    textureIDs[ID::Clay] = createBlockTextures(TextureID::Clay);
    textureIDs[ID::Gravel] = createBlockTextures(TextureID::Gravel);
    textureIDs[ID::Sand] = createBlockTextures(TextureID::Sand);
    textureIDs[ID::Snow] = createBlockTextures(TextureID::Snow);
    textureIDs[ID::Stone] = createBlockTextures(TextureID::Stone);
    textureIDs[ID::OakLog] = createBlockTextures(TextureID::OakLogTop, TextureID::OakLogSide);
    textureIDs[ID::OakLeaves] = createBlockTextures(TextureID::OakLeaves);
    textureIDs[ID::FallLeaves] = createBlockTextures(TextureID::FallLeaves);
    textureIDs[ID::Glass] = createBlockTextures(TextureID::Glass);
    textureIDs[ID::Water] = createBlockTextures(TextureID::Water);
    textureIDs[ID::Null] = createBlockTextures(TextureID::ErrorTexture);
    return textureIDs;
  }

  static eng::EnumArray<std::filesystem::path, TextureID> computeTexturePaths()
  {
    eng::EnumArray<std::filesystem::path, TextureID> texturePaths;

    std::filesystem::path textureFolder = "assets/textures";
    std::filesystem::path tileFolder = textureFolder / "voxel-pack/PNG/Tiles";

    texturePaths[TextureID::GrassTop] = tileFolder / "grass_top.png";
    texturePaths[TextureID::GrassSide] = tileFolder / "dirt_grass.png";
    texturePaths[TextureID::Dirt] = tileFolder / "dirt.png";
    texturePaths[TextureID::Clay] = tileFolder / "greysand.png";
    texturePaths[TextureID::Gravel] = tileFolder / "gravel_stone.png";
    texturePaths[TextureID::Sand] = tileFolder / "sand.png";
    texturePaths[TextureID::Snow] = tileFolder / "snow.png";
    texturePaths[TextureID::Stone] = tileFolder / "stone.png";
    texturePaths[TextureID::OakLogTop] = tileFolder / "trunk_top.png";
    texturePaths[TextureID::OakLogSide] = tileFolder / "trunk_side.png";
    texturePaths[TextureID::OakLeaves] = tileFolder / "leaves_transparent.png";
    texturePaths[TextureID::FallLeaves] = tileFolder / "leaves_orange_transparent.png";
    texturePaths[TextureID::Glass] = tileFolder / "glass.png";
    texturePaths[TextureID::Water] = tileFolder / "water_transparent.png";
    texturePaths[TextureID::Invisible] = textureFolder / "Invisible.png";
    texturePaths[TextureID::ErrorTexture] = textureFolder / "Checkerboard.png";

    return texturePaths;
  }



  struct BlockUniformData
  {
    const f32 blockLength = eng::arithmeticCastUnchecked<f32>(length());
  };

  struct BlockProperties
  {
    eng::EnumArray<bool, ID> hasTransparency;
    eng::EnumArray<bool, ID> hasCollision;
    eng::EnumArray<eng::EnumArray<TextureID, eng::math::Direction>, ID> textureIDs;

    BlockProperties()
    {
      hasTransparency = computeTransparencies();
      hasCollision = computeCollisionalities();
      textureIDs = computeTextureIDs();
    }
  };
  static BlockProperties s_Properties;
  
  static constexpr i32 c_UniformBinding = 1;
  static std::once_flag s_InitializedFlag;

  static eng::EnumArray<eng::math::Float4, TextureID> s_TextureAverageColors;
  static eng::EnumArray<std::filesystem::path, TextureID> s_TexturePaths;
  static std::shared_ptr<eng::TextureArray> s_TextureArray;
  static std::unique_ptr<eng::Uniform> s_Uniform;
  static const BlockUniformData s_BlockUniformData{};

  static void initialize()
  {
    std::call_once(s_InitializedFlag, []()
    {
      s_Uniform = eng::Uniform::Create(c_UniformBinding, sizeof(BlockUniformData));
      s_Uniform->set(&s_BlockUniformData, sizeof(BlockUniformData));

      s_TexturePaths = computeTexturePaths();

      s_TextureArray = eng::TextureArray::Create(16, 128);
      for (TextureID texture : Textures())
      {
        if (s_TexturePaths[texture] == "")
        {
          ENG_ERROR("Block texture {0} has not been assign a path!", std::underlying_type_t<TextureID>(texture));
          texture = TextureID::ErrorTexture;
        }

        eng::Image textureImage(s_TexturePaths[texture]);
        s_TextureArray->addTexture(textureImage);

        eng::math::Float4 textureAverageColor = textureImage.averageColor();
        textureAverageColor.a = 1.0f;
        s_TextureAverageColors[texture] = textureAverageColor;
      }
    });
  }

  std::shared_ptr<eng::TextureArray> getTextureArray()
  {
    initialize();
    return s_TextureArray;
  }



  TextureID Type::texture(eng::math::Direction face) const
  {
    return s_Properties.textureIDs[m_TypeID][face];
  }

  bool Type::hasTransparency() const
  {
    return s_Properties.hasTransparency[m_TypeID];
  }

  bool Type::hasCollision() const
  {
    return s_Properties.hasCollision[m_TypeID];
  }



  Light::Light()
    : Light(0) {}
  Light::Light(i8 sunlight)
    : m_Sunlight(sunlight)
  {
    ENG_ASSERT(eng::debug::boundsCheck(sunlight, 0, MaxValue() + 1), "Invalid value for sunlight!");
  }

  bool Light::operator==(Light other) const
  {
    return m_Sunlight == other.m_Sunlight;
  }

  i8 Light::sunlight() const
  {
    return m_Sunlight;
  }
}