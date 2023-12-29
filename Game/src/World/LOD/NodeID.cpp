#include "GMpch.h"
#include "NodeID.h"
#include "GlobalParameters.h"
#include "Indexing/Operations.h"
#include "World/Chunk/Chunk.h"

namespace newLod
{
  bool NodeID::operator==(const NodeID& other) const = default;

  const GlobalIndex& NodeID::anchor() const { return m_Anchor; }
  globalIndex_t NodeID::depth() const { return m_Depth; }

  i32 NodeID::lodLevel() const { return param::MaxNodeDepth() - depth(); }
  globalIndex_t NodeID::size() const { return eng::math::pow2<globalIndex_t>(lodLevel()); }
  length_t NodeID::length() const { return size() * Chunk::Length(); }
  length_t NodeID::boundingSphereRadius() const { return std::numbers::sqrt3_v<length_t> * length() / 2; }
  GlobalBox NodeID::boundingBox() const { return { m_Anchor, m_Anchor + (size() - 1) }; }

  eng::math::Vec3 NodeID::anchorPosition(const GlobalIndex& originIndex) const { return indexPosition(anchor(), originIndex); }
  eng::math::Vec3 NodeID::center(const GlobalIndex& originIndex) const { return anchorPosition(originIndex) + length() / 2; }

  NodeID NodeID::child(const BlockIndex& childIndex) const
  {
    globalIndex_t childSize = size() / 2;
    GlobalIndex childAnchor = anchor() + childSize * childIndex.upcast<globalIndex_t>();
    return NodeID(childAnchor, depth() + 1);
  }

  BlockIndex NodeID::childIndex(const GlobalIndex& index) const
  {
    ENG_ASSERT(boundingBox().encloses(index), "Node does not enclose the given index!");

    globalIndex_t childSize = size() / 2;
    GlobalIndex childIndex = (index - anchor()) / childSize;
    return childIndex.checkedCast<blockIndex_t>();
  }
}