#pragma once
#include "Indexing/Definitions.h"

namespace newLod
{
  class NodeID
  {
    GlobalIndex m_Anchor;
    globalIndex_t m_Depth;

  public:
    NodeID(const GlobalIndex& anchor, globalIndex_t depth);

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
  };

  class MeshID
  {
    NodeID m_NodeID;
    std::optional<eng::math::Direction> m_Face;

  public:
    MeshID(const NodeID& node);
    MeshID(const NodeID& node, eng::math::Direction face);

    bool operator==(const MeshID& other) const;

    const NodeID& nodeID() const;
    std::optional<eng::math::Direction> face() const;
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

  template<>
  struct hash<newLod::MeshID>
  {
    uSize operator()(const newLod::MeshID& meshID) const
    {
      static constexpr underlying_type_t<eng::math::Direction> stride = eng::enumRange<eng::math::Direction>() + 1;
      underlying_type_t<eng::math::Direction> offset = meshID.face() ? 1 + eng::toUnderlying(*meshID.face()) : 0;
      return stride * hash<GlobalIndex>()(meshID.nodeID().anchor()) + offset;
    }
  };
}