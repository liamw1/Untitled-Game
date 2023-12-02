#include "GMpch.h"
#include "NewLOD.h"

namespace newLod
{
  static constexpr u64 c_RootNodeSize = eng::math::pow2<u64>(Octree::MaxDepth());
  static constexpr GlobalIndex c_RootNodeAnchor = -eng::arithmeticCast<globalIndex_t>(c_RootNodeSize / 2) * GlobalIndex(1);
  static constexpr BlockBox c_NodeChildIndexBounds = BlockBox(0, 1);
  static constexpr GlobalBox c_Bounds = GlobalBox(c_RootNodeAnchor, -c_RootNodeAnchor - 1);



  Octree::Node::Node()
    : children(c_NodeChildIndexBounds, eng::AllocationPolicy::Deferred) {}

  bool Octree::Node::isLeaf() const { return !children; }



  Octree::Octree() = default;

  std::optional<NodeID> Octree::find(const GlobalIndex& index) const
  {
    if (!c_Bounds.encloses(index))
      return std::nullopt;
    return findImpl(m_Root, rootNodeID(), index);
  }

  void Octree::divide(const GlobalIndex& index)
  {
    if (c_Bounds.encloses(index))
      divideImpl(m_Root, rootNodeID(), index);
  }

  std::vector<NodeID> Octree::getLeafNodes() const
  {
    std::vector<NodeID> leafNodes;
    getLeafNodesImpl(leafNodes, m_Root, rootNodeID());
    return leafNodes;
  }

  eng::EnumBitMask<eng::math::Direction> Octree::transitionNeighbors(const NodeID& nodeInfo) const
  {
    eng::EnumArray<GlobalIndex, eng::math::Direction> offsets =
    { { eng::math::Direction::West,   {-1,               0,               0} },
      { eng::math::Direction::East,   {nodeInfo.size(),  0,               0} },
      { eng::math::Direction::South,  { 0,              -1,               0} },
      { eng::math::Direction::North,  { 0, nodeInfo.size(),               0} },
      { eng::math::Direction::Bottom, { 0,               0,              -1} },
      { eng::math::Direction::Top,    { 0,               0, nodeInfo.size()} } };

    eng::EnumBitMask<eng::math::Direction> transitionNeighbors;
    for (eng::math::Direction face : eng::math::Directions())
    {
      std::optional<NodeID> neighborID = find(nodeInfo.anchor() + offsets[face]);
      if (!neighborID)
        continue;

      if (neighborID->lodLevel() - nodeInfo.lodLevel() == 1)
        transitionNeighbors.set(face);
      else if (neighborID->lodLevel() - nodeInfo.lodLevel() > 1)
        ENG_WARN("LOD neighbor is more than one level lower resolution!");
    }
    return transitionNeighbors;
  }

  NodeID Octree::rootNodeID() const
  {
    return NodeID(c_RootNodeAnchor, 0);
  }

  void Octree::splitNode(Node& leafNode, const NodeID& nodeInfo)
  {
    ENG_ASSERT(nodeInfo.depth() < MaxDepth(), "Node is already at max depth!");
    if (!leafNode.isLeaf())
      return;

    leafNode.children.allocate();
  }

  void Octree::combineNode(Node& branchNode)
  {
    if (branchNode.isLeaf())
      return;
    
    branchNode.children.forEach([this](Node& child) { combineNode(child); });
    branchNode.children.clear();
  }

  std::optional<NodeID> Octree::findImpl(const Node& branch, const NodeID& branchInfo, const GlobalIndex& index) const
  {
    if (branch.isLeaf())
      return branchInfo;

    globalIndex_t childSize = branchInfo.size() / 2;
    GlobalIndex childIndex = (index - branchInfo.anchor()) / childSize;
    GlobalIndex childAnchor = branchInfo.anchor() + childSize * childIndex;
    return findImpl(branch.children(childIndex.uncheckedCast<blockIndex_t>()), NodeID(childAnchor, branchInfo.depth() + 1), index);
  }

  void Octree::getLeafNodesImpl(std::vector<NodeID>& leafNodes, const Node& node, const NodeID& nodeInfo) const
  {
    if (node.isLeaf())
    {
      leafNodes.push_back(nodeInfo);
      return;
    }

    c_NodeChildIndexBounds.forEach([this, &leafNodes, &node, &nodeInfo](const BlockIndex& childIndex)
    {
      globalIndex_t childSize = nodeInfo.size() / 2;
      GlobalIndex childAnchor = nodeInfo.anchor() + childSize * childIndex.upcast<globalIndex_t>();
      getLeafNodesImpl(leafNodes, node.children(childIndex), NodeID(childAnchor, nodeInfo.depth() + 1));
    });
  }

  void Octree::divideImpl(Node& node, const NodeID& nodeInfo, const GlobalIndex& index)
  {
    if (!node.isLeaf() || nodeInfo.lodLevel() == 0)
      return;

    globalIndex_t splitRange = 2 * nodeInfo.size() - 1 + param::RenderDistance();
    GlobalBox splitRangeBoundingBox(index - splitRange, index + splitRange);
    GlobalBox intersection = GlobalBox::Intersection(splitRangeBoundingBox, nodeInfo.boundingBox());
    if (!intersection.valid())
      return;

    splitNode(node, nodeInfo);

    c_NodeChildIndexBounds.forEach([this, &node, &nodeInfo, &index](const BlockIndex& childIndex)
    {
      globalIndex_t childSize = nodeInfo.size() / 2;
      GlobalIndex childAnchor = nodeInfo.anchor() + childSize * childIndex.upcast<globalIndex_t>();
      divideImpl(node.children(childIndex), NodeID(childAnchor, nodeInfo.depth() + 1), index);
    });
  }
}
