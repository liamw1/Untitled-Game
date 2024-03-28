#pragma once
#include "BiomeIDs.h"

#include <Engine.h>

namespace biome
{
  struct TableBranchNode;
  struct TableLeafNode;
  using TableNode = std::variant<TableBranchNode, TableLeafNode>;

  struct TableBranchNode
  {
    eng::UniqueArray<TableNode, 2> children;
    length_t dividingPlane;
    Property dividingAxis;

    TableBranchNode();
    TableBranchNode(eng::UniqueArray<TableNode, 2>&& _children, length_t _dividingPlane, Property _dividingAxis);
  };

  struct TableLeafNode
  {
    eng::EnumArray<length_t, Property> biomeProperties;
    ID biome;
  };

  using PropertyVector = eng::EnumArray<length_t, Property>;

  class PropertyBox
  {
    eng::EnumArray<eng::math::Interval<length_t>, Property> m_Bounds;

  public:
    constexpr PropertyBox()
      : PropertyBox(0, 0) {}
    constexpr PropertyBox(length_t min, length_t max)
      : m_Bounds(min, max) {}

    ENG_DEFINE_CONSTEXPR_ITERATORS(m_Bounds);

    constexpr eng::math::Interval<length_t>& operator[](Property index) { ENG_MUTABLE_VERSION(operator[], index); }
    constexpr const eng::math::Interval<length_t>& operator[](Property index) const { return m_Bounds[index]; }

    constexpr bool contains(const PropertyVector& biomeProperties) const
    {
      for (Property property : eng::EnumIterator<Property>())
        if (biomeProperties[property] < m_Bounds[property].min || biomeProperties[property] > m_Bounds[property].max)
          return false;
      return true;
    }
  };
}
