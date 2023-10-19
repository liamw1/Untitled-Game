#pragma once

template<typename ComponentType, int ComponentCount>
class CompoundType
{
public:
  struct Component
  {
    ComponentType type = ComponentType();
    float weight = 0.0;
  };

  constexpr CompoundType()
  {
    for (Component& component : m_Components)
      component = { ComponentType(), 0.0f};
  }
  constexpr CompoundType(ComponentType initialValue)
  {
    m_Components.front() = { initialValue, 1.0f };
    for (int i = 1; i < ComponentCount; ++i)
      m_Components[i] = { ComponentType(), 0.0f};
  }

  template<int N>
  CompoundType(const std::array<Component, N>& components)
  {
    float sumOfWeights = 0.0;
    for (int i = 0; i < std::min(N, ComponentCount); ++i)
    {
      m_Components[i] = components[i];
      sumOfWeights += components[i].weight;
    }

    for (Component& component : m_Components)
      component.weight /= sumOfWeights;
  }

  ComponentType getPrimary() const { return m_Components.front().type; };

  const Component& operator[](int index) const
  {
    ENG_ASSERT(0 <= index && index < ComponentCount, "Index out of bounds!");
    return m_Components[index];
  }

  CompoundType operator+(const CompoundType& other) const
  {
    CompoundType sum = *this;
    return sum += other;
  }
  CompoundType operator*(float x) const
  {
    CompoundType result = *this;
    return result *= x;
  }

  CompoundType& operator+=(const CompoundType& other)
  {
    std::array<Component, ComponentCount> mergedComposition{};

    int i = 0, j = 0;
    for (int k = 0; k < ComponentCount; ++k)
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

    // NOTE: Can use i,j here to determine lost weights

    m_Components = mergedComposition;
    return *this;
  }

  CompoundType& operator*=(float x)
  {
    ENG_ASSERT(x >= 0.0, "Compound block cannot be multiplied by a negative number!");

    for (int i = 0; i < ComponentCount; ++i)
      m_Components[i].weight *= x;

    return *this;
  }

  static constexpr int MaxTypes() { return ComponentCount; }

private:
  std::array<Component, ComponentCount> m_Components{};
};

template<typename ComponentType, int ComponentCount>
CompoundType<ComponentType, ComponentCount> operator*(float x, const CompoundType<ComponentType, ComponentCount>& compoundType)
{
  return compoundType * x;
}