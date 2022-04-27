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
  std::array<Component, s_NumTypes> mergedComposition{};

  int i = 0, j = 0;
  for (int k = 0; k < s_NumTypes; ++k)
  {
    if (m_Components[i].weight > other.m_Components[j].weight)
    {
      mergedComposition[k] = m_Components[i];
      i++;
    }
    else
    {
      mergedComposition[k] = other.m_Components[j];
      j++;
    }
  }

  m_Components = mergedComposition;
  return *this;
}

Block::CompoundType& Block::CompoundType::operator*=(float x)
{
  EN_ASSERT(x >= 0.0, "Compound block cannot be multiplied by a negative number!");

  for (int i = 0; i < s_NumTypes; ++i)
    m_Components[i].weight *= x;

  return *this;
}