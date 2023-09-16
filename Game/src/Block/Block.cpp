#include "GMpch.h"
#include "Block.h"

namespace Block
{
  struct BlockUniformData
  {
    const float blockLength = static_cast<float>(Length());
  };
  
  static constexpr int c_UniformBinding = 1;
  static inline int s_BlocksInitialized = 0;
  static inline bool s_Initialized = false;

  static inline Engine::EnumArray<bool, Block::ID> s_HasTransparency;
  static inline Engine::EnumArray<bool, Block::ID> s_HasCollision;
  static inline Engine::EnumArray<DirectionArray<TextureID>, Block::ID> s_TextureIDs;
  static inline Engine::EnumArray<std::filesystem::path, Block::TextureID> s_TexturePaths;
  static inline std::shared_ptr<Engine::TextureArray> s_TextureArray;
  static inline std::unique_ptr<Engine::Uniform> s_Uniform;
  static inline const BlockUniformData s_BlockUniformData{};
  
  static void assignTextures(ID block, const DirectionArray<TextureID>& faceTextures)
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

    s_TextureArray = Engine::TextureArray::Create(16, 128);
    for (TextureID texture : Textures())
    {
      if (s_TexturePaths[texture] == "")
      {
        EN_ERROR("Block texture {0} has not been assign a path!", std::underlying_type_t<Block::TextureID>(texture));
        texture = TextureID::ErrorTexture;
      }
      s_TextureArray->addTexture(s_TexturePaths[texture].string());
    }

    static constexpr int numBlockTypes = Engine::EnumRange<Block::ID>();
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



  void Initialize()
  {
    s_Uniform = Engine::Uniform::Create(c_UniformBinding, sizeof(BlockUniformData));
    s_Uniform->set(&s_BlockUniformData, sizeof(BlockUniformData));

    assignTextures();
    assignTransparency();
    assignCollisionality();

    s_Initialized = true;
  }

  std::shared_ptr<Engine::TextureArray> GetTextureArray()
  {
    EN_ASSERT(s_Initialized, "Blocks have not been initialized!");
    return s_TextureArray;
  }



  TextureID Type::texture(Direction face) const
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
    EN_ASSERT(Engine::Debug::BoundsCheck(sunlight, 0, MaxValue() + 1), "Invalid value for sunlight!");
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