#include "GMpch.h"
#include "CompoundBlockType.h"

Block::Type Block::CompoundType::getType(int index) const
{
  EN_ASSERT(0 <= index && index < s_NumTypes, "Index out of bounds!");
  return m_Components[index].type;
}

float Block::CompoundType::getWeight(int index) const
{
  EN_ASSERT(0 <= index && index < s_NumTypes, "Index out of bounds!");
  return m_Components[index].weight;
}

Block::CompoundType Block::CompoundType::operator+(const CompoundType& other) const
{
  CompoundType sum = *this;
  return sum += other;
}

Block::CompoundType Block::CompoundType::operator*(float x) const
{
  CompoundType result = *this;
  return result *= x;
}

Block::CompoundType& Block::CompoundType::operator+=(const CompoundType& other)
{
  // NOTE: This function can probably be made more efficient

  std::array<Component, 2 * s_NumTypes> combinedComposition{};
  for (int i = 0; i < s_NumTypes; ++i)
    combinedComposition[i] = m_Components[i];

  // Add like componets together and insert new components
  for (int i = 0; i < s_NumTypes; ++i)
  {
    bool otherComponentPresent = false;
    const Component& otherComponent = other.m_Components[i];
    for (int j = 0; j < s_NumTypes; ++j)
      if (combinedComposition[j].type == otherComponent.type)
      {
        combinedComposition[j].weight += otherComponent.weight;
        otherComponentPresent = true;
        break;
      }

    if (!otherComponentPresent)
      combinedComposition[s_NumTypes + i] = otherComponent;
  }

  std::sort(combinedComposition.begin(), combinedComposition.end(), [](Component a, Component b) { return a.weight > b.weight; });

  for (int i = 0; i < s_NumTypes; ++i)
    m_Components[i] = combinedComposition[i];

  return *this;
}

Block::CompoundType& Block::CompoundType::operator*=(float x)
{
  EN_ASSERT(x >= 0.0, "Compound block cannot be multiplied by a negative number!");

  for (int i = 0; i < s_NumTypes; ++i)
    m_Components[i].weight *= x;

  return *this;
}