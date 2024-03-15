#include "GMpch.h"
#include "Block.h"

namespace block
{
  static constexpr eng::EnumArray<bool, ID> computeTransparencies()
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

  static constexpr eng::EnumArray<bool, ID> computeCollisionalities()
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

  static constexpr eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID westTexture,   TextureID eastTexture,
                                                                                       TextureID southTexture,  TextureID northTexture,
                                                                                       TextureID bottomTexture, TextureID topTexture)
  {
    return { { eng::math::Direction::West,   westTexture   }, { eng::math::Direction::East,  eastTexture  },
             { eng::math::Direction::South,  southTexture  }, { eng::math::Direction::North, northTexture },
             { eng::math::Direction::Bottom, bottomTexture }, { eng::math::Direction::Top,   topTexture   } };
  }

  static constexpr eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID topTexture, TextureID sideTextures, TextureID bottomTexture)
  {
    return createBlockTextures(sideTextures, sideTextures, sideTextures, sideTextures, bottomTexture, topTexture);
  }

  static constexpr eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID topBotTextures, TextureID sideTextures)
  {
    return createBlockTextures(topBotTextures, sideTextures, topBotTextures);
  }

  static constexpr eng::EnumArray<TextureID, eng::math::Direction> createBlockTextures(TextureID faceTextures)
  {
    return createBlockTextures(faceTextures, faceTextures);
  }

  static constexpr eng::EnumArray<eng::EnumArray<TextureID, eng::math::Direction>, ID> computeTextureIDs()
  {
    return { { ID::Air, createBlockTextures(TextureID::Invisible) },
             { ID::Grass, createBlockTextures(TextureID::GrassTop, TextureID::GrassSide, TextureID::Dirt) },
             { ID::Dirt, createBlockTextures(TextureID::Dirt) },
             { ID::Clay, createBlockTextures(TextureID::Clay) },
             { ID::Gravel, createBlockTextures(TextureID::Gravel) },
             { ID::Sand, createBlockTextures(TextureID::Sand) },
             { ID::Snow, createBlockTextures(TextureID::Snow) },
             { ID::Stone, createBlockTextures(TextureID::Stone) },
             { ID::OakLog, createBlockTextures(TextureID::OakLogTop, TextureID::OakLogSide) },
             { ID::OakLeaves, createBlockTextures(TextureID::OakLeaves) },
             { ID::FallLeaves, createBlockTextures(TextureID::FallLeaves) },
             { ID::Glass, createBlockTextures(TextureID::Glass) },
             { ID::Water, createBlockTextures(TextureID::Water) },
             { ID::Null, createBlockTextures(TextureID::ErrorTexture) } };
  }

  static eng::EnumArray<std::filesystem::path, TextureID> computeTexturePaths()
  {
    std::filesystem::path textureFolder = "assets/textures";
    std::filesystem::path tileFolder = textureFolder / "voxel-pack/PNG/Tiles";

    return { { TextureID::GrassTop, tileFolder / "grass_top.png" },
             { TextureID::GrassSide, tileFolder / "dirt_grass.png" },
             { TextureID::Dirt, tileFolder / "dirt.png" },
             { TextureID::Clay, tileFolder / "greysand.png" },
             { TextureID::Gravel, tileFolder / "gravel_stone.png" },
             { TextureID::Sand, tileFolder / "sand.png" },
             { TextureID::Snow, tileFolder / "snow.png" },
             { TextureID::Stone, tileFolder / "stone.png" },
             { TextureID::OakLogTop, tileFolder / "trunk_top.png" },
             { TextureID::OakLogSide, tileFolder / "trunk_side.png" },
             { TextureID::OakLeaves, tileFolder / "leaves_transparent.png" },
             { TextureID::FallLeaves, tileFolder / "leaves_orange_transparent.png" },
             { TextureID::Glass, tileFolder / "glass.png" },
             { TextureID::Water, tileFolder / "water_transparent.png" },
             { TextureID::Invisible, textureFolder / "Invisible.png" },
             { TextureID::ErrorTexture, textureFolder / "Checkerboard.png" } };
  }



  struct BlockUniformData
  {
    const f32 blockLength = lengthF();
  };

  struct BlockProperties
  {
    eng::EnumArray<bool, ID> hasTransparency;
    eng::EnumArray<bool, ID> hasCollision;
    eng::EnumArray<eng::EnumArray<TextureID, eng::math::Direction>, ID> textureIDs;

    constexpr BlockProperties()
    {
      hasTransparency = computeTransparencies();
      hasCollision = computeCollisionalities();
      textureIDs = computeTextureIDs();
    }
  };
  static constexpr BlockProperties s_Properties;
  
  static constexpr i32 c_UniformBinding = 1;
  static constexpr i32 c_SSBOBinding = 0;
  static constexpr BlockUniformData c_BlockUniformData;

  static eng::EnumArray<eng::EnumArray<eng::math::Float4, eng::math::Direction>, ID> s_BlockAverageColors;
  static eng::EnumArray<std::filesystem::path, TextureID> s_TexturePaths;
  static std::shared_ptr<eng::TextureArray> s_TextureArray;
  static std::unique_ptr<eng::Uniform> s_Uniform;
  static std::unique_ptr<eng::ShaderBufferStorage> s_SSBO;

  static void initialize()
  {
    static bool initialized = []()
    {
      s_Uniform = std::make_unique<eng::Uniform>("Block", c_UniformBinding, sizeof(BlockUniformData));
      s_Uniform->write(c_BlockUniformData);

      s_TexturePaths = computeTexturePaths();

      s_TextureArray = eng::TextureArray::Create(16, 128);
      eng::EnumArray<eng::math::Float4, TextureID> textureAverageColors;
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
        textureAverageColors[texture] = textureAverageColor;
      }

      for (ID blockID : IDs())
        for (eng::math::Direction blockFace : eng::math::Directions())
          s_BlockAverageColors[blockID][blockFace] = textureAverageColors[s_Properties.textureIDs[blockID][blockFace]];

      eng::mem::RenderData textureAverageColorsData(s_BlockAverageColors);
      s_SSBO = std::make_unique<eng::ShaderBufferStorage>(c_SSBOBinding, textureAverageColorsData.size());
      s_SSBO->write(textureAverageColorsData);

      return true;
    }();
  }

  std::shared_ptr<eng::TextureArray> getTextureArray()
  {
    initialize();
    return s_TextureArray;
  }

  void bindAverageColorSSBO()
  {
    initialize();
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
    ENG_ASSERT(eng::withinBounds(sunlight, 0, MaxValue() + 1), "Invalid value for sunlight!");
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