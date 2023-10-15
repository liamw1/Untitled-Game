#pragma once
#include "BlockIDs.h"
#include "World/Indexing.h"

namespace block
{
  constexpr length_t length() { return 0.5_m; }
  constexpr float lengthF() { return static_cast<float>(length()); }

  void initialize();
  std::shared_ptr<eng::TextureArray> getTextureArray();

  class Type
  {
  public:
    constexpr Type()
      : Type(block::ID::Null) {}
    constexpr Type(block::ID blockType)
      : m_TypeID(blockType) {}
  
    constexpr bool operator==(Type other) const { return m_TypeID == other.m_TypeID; }
    constexpr bool operator==(block::ID blockType) const { return m_TypeID == blockType; }
  
    block::TextureID texture(eng::math::Direction face) const;
  
    bool hasTransparency() const;
    bool hasCollision() const;
  
  private:
    block::ID m_TypeID;
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