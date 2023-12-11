#pragma once
#include "LODHelpers.h"
#include "World/Terrain.h"

namespace newLod
{
  class LODManager
  {
    std::shared_ptr<eng::thread::AsyncMultiDrawArray<DrawCommand>> m_MultiDrawArray;

    // Multi-threading
    std::shared_ptr<eng::thread::ThreadPool> m_ThreadPool;
    eng::thread::WorkSet<GlobalIndex, void> m_MeshingWork;

    // LOD data
    Node m_Root;

  public:
    LODManager();

    void render();

  private:
    void meshLOD(const NodeID& nodeID);

    eng::EnumBitMask<eng::math::Direction> transitionNeighbors(const NodeID& nodeInfo) const;

    /*
      \returns The leaf node ID that contains the given index.
               Will return std::nullopt if index is outside of octree bounds.
    */
    std::optional<NodeID> find(const GlobalIndex& index) const;
    std::optional<NodeID> findImpl(const Node& branch, const NodeID& branchInfo, const GlobalIndex& index) const;

    // TODO: Remove
    std::vector<NodeID> getLeafNodes() const;
    void getLeafNodesImpl(std::vector<NodeID>& leafNodes, const Node& node, const NodeID& nodeInfo) const;

    void divide(const GlobalIndex& index);
    void divideImpl(Node& node, const NodeID& nodeInfo, const GlobalIndex& index);

    void divideTask(const NodeID& nodeInfo);

    bool replaceLeafNode(Node&& node, const NodeID& nodeInfo);
  };
}