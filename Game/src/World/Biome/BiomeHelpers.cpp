#include "GMpch.h"
#include "BiomeHelpers.h"

namespace biome
{
  TableBranchNode::TableBranchNode()
    : children(eng::AllocationPolicy::Deferred), dividingPlane(0.0), dividingAxis(Property::First) {}

  TableBranchNode::TableBranchNode(eng::UniqueArray<TableNode, 2>&& _children, f64 _dividingPlane, Property _dividingAxis)
    : children(std::move(_children)), dividingPlane(_dividingPlane), dividingAxis(_dividingAxis) {}

  bool PropertyBox::contains(const eng::EnumArray<f64, Property>& biomeProperties) const
  {
    for (Property property : eng::EnumIterator<Property>())
      if (biomeProperties[property] < m_Bounds[property].min || biomeProperties[property] > m_Bounds[property].max)
        return false;
    return true;
  }
}