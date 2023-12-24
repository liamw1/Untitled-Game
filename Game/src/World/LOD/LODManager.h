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

    void update();

  private:
    eng::EnumArray<std::optional<NodeID>, eng::math::Direction> neighborQuery(const NodeID& nodeInfo) const;

    eng::EnumBitMask<eng::math::Direction> transitionNeighbors(const NodeID& nodeInfo) const;

    void updateImpl(bool& stateChanged, Node& branch, const NodeID& branchInfo, const GlobalIndex& originIndex);

    bool tryDivide(Node& leafNode, const NodeID& nodeInfo, const GlobalIndex& originIndex);
    bool tryCombine(Node& parentNode, const NodeID& nodeInfo, const GlobalIndex& originIndex);

    /*
      \returns The leaf node ID that contains the given index.
               Will return std::nullopt if index is outside of octree bounds.
    */
    std::optional<NodeID> find(const GlobalIndex& index) const;
    std::optional<NodeID> findImpl(const Node& branch, const NodeID& branchInfo, const GlobalIndex& index) const;

    std::optional<DrawCommand> createNewMesh(Node& node, const NodeID& nodeID);
    std::optional<DrawCommand> createAdjustedMesh(const Node& node, const NodeID& nodeInfo) const;

    void createAdjustedMeshesInRegion(std::vector<DrawCommand>& drawCommands, const GlobalBox& region) const;
    void createAdjustedMeshesInRegionImpl(std::vector<DrawCommand>& drawCommands, const Node& branch, const NodeID& branchInfo, const GlobalBox& region) const;
  };
}