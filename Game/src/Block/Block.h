#pragma once
#include "BlockIDs.h"
#include "World/Indexing.h"

namespace Block
{
  constexpr length_t Length() { return 0.5_m; }
  constexpr float LengthF() { return static_cast<float>(Length()); }

  void Initialize();
  std::shared_ptr<Engine::TextureArray> GetTextureArray();

  class Type
  {
  public:
    constexpr Type()
      : Type(Block::ID::Null) {}
    constexpr Type(Block::ID blockType)
      : m_TypeID(blockType) {}
  
    constexpr bool operator==(Type other) const { return m_TypeID == other.m_TypeID; }
    constexpr bool operator==(Block::ID blockType) const { return m_TypeID == blockType; }
  
    Block::TextureID texture(Direction face) const;
  
    bool hasTransparency() const;
    bool hasCollision() const;
  
  private:
    Block::ID m_TypeID;
  };

  class Light
  {
  public:
    Light();
    Light(int8_t sunlight);

    bool operator==(Light other) const;

    int8_t sunlight() const;

    static constexpr int MaxValue() { return 15; }

  private:
    int8_t m_Sunlight;
  };
};