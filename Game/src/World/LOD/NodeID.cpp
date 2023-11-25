#include "GMpch.h"
#include "NodeID.h"
#include "NewLOD.h"
#include "GlobalParameters.h"
#include "Indexing/Operations.h"
#include "World/Chunk/Chunk.h"

namespace newLod
{
  NodeID::NodeID(const GlobalIndex& anchor, globalIndex_t depth)
    : m_Anchor(anchor), m_Depth(depth) {}

  bool NodeID::operator==(const NodeID& other) const = default;

  const GlobalIndex& NodeID::anchor() const { return m_Anchor; }
  globalIndex_t NodeID::depth() const { return m_Depth; }

  i32 NodeID::lodLevel() const { return Octree::LODLevel(depth()); }
  globalIndex_t NodeID::size() const { return eng::math::pow2<globalIndex_t>(lodLevel()); }
  length_t NodeID::length() const { return size() * Chunk::Length(); }
  length_t NodeID::boundingSphereRadius() const { return std::numbers::sqrt3_v<length_t> * length() / 2; }

  eng::math::Vec3 NodeID::anchorPosition(const GlobalIndex& originIndex) const { return indexPosition(anchor(), originIndex); }
  eng::math::Vec3 NodeID::center(const GlobalIndex& originIndex) const { return anchorPosition(originIndex) + length() / 2; }



  MeshID::MeshID(const NodeID& node)
    : m_NodeID(node), m_Face(std::nullopt) {}
  MeshID::MeshID(const NodeID& node, eng::math::Direction face)
    : m_NodeID(node), m_Face(face) {}

  bool MeshID::operator==(const MeshID& other) const = default;

  const NodeID& MeshID::nodeID() const { return m_NodeID; }
  std::optional<eng::math::Direction> MeshID::face() const { return m_Face; }
}