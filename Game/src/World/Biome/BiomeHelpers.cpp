#include "GMpch.h"
#include "BiomeHelpers.h"

namespace biome
{
  TableBranchNode::TableBranchNode()
    : children(eng::AllocationPolicy::Deferred), dividingPlane(0_m), dividingAxis(Property::First) {}

  TableBranchNode::TableBranchNode(eng::UniqueArray<TableNode, 2>&& _children, length_t _dividingPlane, Property _dividingAxis)
    : children(std::move(_children)), dividingPlane(_dividingPlane), dividingAxis(_dividingAxis) {}
}