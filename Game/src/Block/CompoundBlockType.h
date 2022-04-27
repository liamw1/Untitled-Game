#pragma once
#include "Block.h"

namespace Block
{
  class CompoundType
  {
  public:
    CompoundType() = default;
    constexpr CompoundType(Block::Type blockType)
      : m_Components({})
    {
      m_Components[0] = { blockType, 1.0f };
    }

    Block::Type getPrimary() const { return m_Components[0].type; };

    Block::Type getType(int index) const;
    float getWeight(int index) const;

    CompoundType operator+(const CompoundType& other) const;
    CompoundType operator*(float x) const;

    CompoundType& operator+=(const CompoundType& other);
    CompoundType& operator*=(float x);

    static constexpr int MaxTypes() { return s_NumTypes; }

  private:
    struct Component
    {
      Block::Type type = Block::Type::Air;
      float weight = 0.0;
    };
    static constexpr int s_NumTypes = 4;

    std::array<Component, s_NumTypes> m_Components{};
  };
}