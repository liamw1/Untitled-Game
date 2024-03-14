#pragma once
#include "BlockIDs.h"
#include "GlobalParameters.h"

namespace block
{
  constexpr length_t length() { return param::BlockLength(); }
  constexpr f32 lengthF() { return eng::arithmeticCastUnchecked<f32>(length()); }

  std::shared_ptr<eng::TextureArray> getTextureArray();
  void bindAverageColorSSBO();

  class Type
  {
    ID m_TypeID;

  public:
    constexpr Type()
      : Type(ID::Null) {}
    constexpr Type(ID blockType)
      : m_TypeID(blockType) {}
  
    constexpr bool operator==(Type other) const { return m_TypeID == other.m_TypeID; }
    constexpr bool operator==(ID blockType) const { return m_TypeID == blockType; }

    constexpr ID id() const { return m_TypeID; }
  
    TextureID texture(eng::math::Direction face) const;
  
    bool hasTransparency() const;
    bool hasCollision() const;
  };

  class Light
  {
    i8 m_Sunlight;

  public:
    Light();
    Light(i8 sunlight);

    bool operator==(Light other) const;

    i8 sunlight() const;

    static constexpr i8 MaxValue() { return 15; }
  };
};