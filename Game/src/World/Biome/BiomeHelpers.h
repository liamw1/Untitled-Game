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
    f64 dividingPlane;
    Property dividingAxis;

    TableBranchNode();
    TableBranchNode(eng::UniqueArray<TableNode, 2>&& _children, f64 _dividingPlane, Property _dividingAxis);
  };

  struct TableLeafNode
  {
    eng::EnumArray<f64, Property> biomeProperties;
    ID biome;
  };

  class PropertyBox
  {
    eng::EnumArray<eng::math::Interval<f64>, Property> m_Bounds;

  public:
    bool contains(const eng::EnumArray<f64, Property>& biomeProperties) const;

  private:
  };
}
