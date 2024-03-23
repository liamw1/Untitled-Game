#include "GMpch.h"
#include "BiomeHelpers.h"

namespace biome
{
  TableNode::TableNode()
    : children(eng::AllocationPolicy::Deferred), dividingAxis(Property::First), dividingPlane(0), biome(ID::First) {}
}