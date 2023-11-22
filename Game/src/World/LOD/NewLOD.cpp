#include "GMpch.h"
#include "NewLOD.h"

namespace newLod
{
  static constexpr u64 c_RootNodeSize = eng::math::pow2<u64>(Octree::MaxDepth());
  static constexpr GlobalIndex c_RootNodeAnchor = -eng::arithmeticCast<globalIndex_t>(c_RootNodeSize / 2) * GlobalIndex(1);
  static constexpr BlockBox c_NodeChildIndexBounds = BlockBox(0, 1);
  static constexpr GlobalBox c_Bounds = GlobalBox(c_RootNodeAnchor, -c_RootNodeAnchor);



  Octree::Node::Node()
    : children(c_NodeChildIndexBounds, eng::AllocationPolicy::Deferred) {}

  bool Octree::Node::isLeaf() const { return children; }



  Octree::Octree() = default;

  std::shared_ptr<RenderData> Octree::find(const GlobalIndex& index)
  {
    if (!c_Bounds.encloses(index))
      return nullptr;
    return findImpl(m_Root, NodeID(c_RootNodeAnchor, 0), index);
  }

  void Octree::splitNode(Node& leafNode, const NodeID& nodeInfo)
  {
    ENG_ASSERT(nodeDepth < MaxDepth(), "Node is already at max depth!");
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

  std::shared_ptr<RenderData> Octree::findImpl(Node& branch, const NodeID& branchInfo, const GlobalIndex& index)
  {
    if (branch.isLeaf())
      return branch.data;

    globalIndex_t childSize = branchInfo.size() / 2;
    GlobalIndex childIndex = (index - branchInfo.anchor()) / childSize;
    GlobalIndex childAnchor = branchInfo.anchor() + childSize * childIndex;
    return findImpl(branch.children(childIndex.uncheckedCast<blockIndex_t>()), NodeID(childAnchor, branchInfo.depth() + 1), index);
  }
}
