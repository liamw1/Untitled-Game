#pragma once
#include "Indexing/Definitions.h"

namespace newLod
{
  class NodeID
  {
    GlobalIndex m_Anchor;
    globalIndex_t m_Depth;

  public:
    constexpr NodeID(const GlobalIndex& anchor, globalIndex_t depth)
      : m_Anchor(anchor), m_Depth(depth) {}

    bool operator==(const NodeID& other) const;

    const GlobalIndex& anchor() const;
    globalIndex_t depth() const;

    i32 lodLevel() const;
    globalIndex_t size() const;
    length_t length() const;
    length_t boundingSphereRadius() const;
    GlobalBox boundingBox() const;

    eng::math::Vec3 anchorPosition(const GlobalIndex& originIndex) const;
    eng::math::Vec3 center(const GlobalIndex& originIndex) const;

    NodeID child(const BlockIndex& childIndex) const;
    BlockIndex childIndex(const GlobalIndex& index) const;
  };
}

namespace std
{
  template<>
  struct hash<newLod::NodeID>
  {
    uSize operator()(const newLod::NodeID& nodeID) const
    {
      static constexpr i32 stride = numeric_limits<globalIndex_t>::digits;
      return stride * hash<GlobalIndex>()(nodeID.anchor()) + nodeID.depth();
    }
  };
}