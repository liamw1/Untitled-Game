#pragma once

template<typename ComponentType, i32 ComponentCount>
class CompoundType
{
public:
  struct Component
  {
    ComponentType type = ComponentType();
    f32 weight = 0.0;
  };

  constexpr CompoundType()
  {
    for (Component& component : m_Components)
      component = { ComponentType(), 0.0f};
  }
  constexpr CompoundType(ComponentType initialValue)
  {
    m_Components.front() = { initialValue, 1.0f };
    for (i32 i = 1; i < ComponentCount; ++i)
      m_Components[i] = { ComponentType(), 0.0f};
  }

  template<i32 N>
  CompoundType(const std::array<Component, N>& components)
  {
    f32 sumOfWeights = 0.0;
    for (i32 i = 0; i < std::min(N, ComponentCount); ++i)
    {
      m_Components[i] = components[i];
      sumOfWeights += components[i].weight;
    }

    for (Component& component : m_Components)
      component.weight /= sumOfWeights;
  }

  ComponentType getPrimary() const { return m_Components.front().type; };

  const Component& operator[](i32 index) const
  {
    ENG_ASSERT(0 <= index && index < ComponentCount, "Index out of bounds!");
    return m_Components[index];
  }

  CompoundType operator+(const CompoundType& right) const
  {
    CompoundType sum = *this;
    return sum += right;
  }
  CompoundType operator*(f32 x) const
  {
    CompoundType result = *this;
    return result *= x;
  }

  CompoundType& operator+=(const CompoundType& right)
  {
    std::array<Component, ComponentCount> mergedComposition{};

    i32 i = 0;
    i32 j = 0;
    for (i32 k = 0; k < ComponentCount; ++k)
    {
      if (m_Components[i].weight > right.m_Components[j].weight)
      {
        mergedComposition[k] = m_Components[i];
        i++;
      }
      else
      {
        mergedComposition[k] = right.m_Components[j];
        j++;
      }
    }

    // NOTE: Can use i,j here to determine lost weights

    m_Components = mergedComposition;
    return *this;
  }

  CompoundType& operator*=(f32 x)
  {
    ENG_ASSERT(x >= 0.0, "Compound block cannot be multiplied by a negative number!");

    for (i32 i = 0; i < ComponentCount; ++i)
      m_Components[i].weight *= x;

    return *this;
  }

  static constexpr i32 MaxTypes() { return ComponentCount; }

private:
  std::array<Component, ComponentCount> m_Components{};
};

template<typename ComponentType, i32 ComponentCount>
CompoundType<ComponentType, ComponentCount> operator*(f32 x, const CompoundType<ComponentType, ComponentCount>& compoundType)
{
  return compoundType * x;
}