#pragma once
#include "NodeID.h"
#include "World/Chunk/Chunk.h"

namespace newLod
{
  struct RenderData
  {
  };

  class Octree
  {
    struct Node
    {
      BlockArrayBox<Node> children;
      std::shared_ptr<RenderData> data;

      Node();
      bool isLeaf() const;
    };

    Node m_Root;

  public:
    Octree();

    /*
      \returns The leaf node that contains the given index.
               Will return nullptr if index is outside of octree bounds.
    */
    std::shared_ptr<RenderData> find(const GlobalIndex& index);

    void divide(const GlobalIndex& index);

    // TODO: Remove
    std::vector<NodeID> getLeafNodes() const;

    static constexpr i32 MaxDepth() { return param::MaxNodeDepth(); }
    static constexpr i32 LODLevel(i32 nodeDepth) { return MaxDepth() - nodeDepth; }

  private:
    void splitNode(Node& leafNode, const NodeID& nodeInfo);
    void combineNode(Node& branchNode);

    std::shared_ptr<RenderData> findImpl(const Node& branch, const NodeID& branchInfo, const GlobalIndex& index);
    void divideImpl(Node& node, const NodeID& nodeInfo, const GlobalIndex& index);
    void getLeafNodesImpl(std::vector<NodeID>& leafNodes, const Node& node, const NodeID& nodeInfo) const;
  };
}