#pragma once
#include "NodeID.h"
#include "GlobalParameters.h"

namespace newLod
{
  class Octree
  {
    struct Node
    {
      BlockArrayBox<Node> children;

      Node();
      bool isLeaf() const;
    };

    Node m_Root;

  public:
    Octree();

    /*
      \returns The leaf node ID that contains the given index.
               Will return std::nullopt if index is outside of octree bounds.
    */
    std::optional<NodeID> find(const GlobalIndex& index) const;

    void divide(const GlobalIndex& index);

    // TODO: Remove
    std::vector<NodeID> getLeafNodes() const;

    eng::EnumBitMask<eng::math::Direction> transitionNeighbors(const NodeID& nodeInfo) const;

    static constexpr i32 MaxDepth() { return param::MaxNodeDepth(); }

  private:
    NodeID rootNodeID() const;

    void splitNode(Node& leafNode, const NodeID& nodeInfo);
    void combineNode(Node& branchNode);

    std::optional<NodeID> findImpl(const Node& branch, const NodeID& branchInfo, const GlobalIndex& index) const;
    void getLeafNodesImpl(std::vector<NodeID>& leafNodes, const Node& node, const NodeID& nodeInfo) const;

    void divideImpl(Node& node, const NodeID& nodeInfo, const GlobalIndex& index);
  };
}