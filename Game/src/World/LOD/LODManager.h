#pragma once
#include "LODHelpers.h"
#include "World/Terrain.h"

namespace newLod
{
  class LODManager
  {
    eng::MultiDrawArray<DrawCommand> m_MultiDrawArray;

    // Multi-threading
    eng::thread::Queue<StateChange> m_StateChangeQueue;
    std::shared_ptr<eng::thread::ThreadPool> m_ThreadPool;
    std::future<void> m_UpdateFuture;

    // LOD data
    Node m_Root;

  public:
    LODManager();
    ~LODManager();

    void render();

    void update();

  private:
    bool updateRecursively(Node& branch, const NodeID& branchInfo, const GlobalIndex& originIndex);

    bool tryDivide(Node& leafNode, const NodeID& nodeInfo, const GlobalIndex& originIndex);
    bool tryCombine(Node& parentNode, const NodeID& nodeInfo, const GlobalIndex& originIndex);

    eng::EnumArray<std::optional<NodeID>, eng::math::Direction> neighborQuery(const NodeID& nodeInfo) const;
    eng::EnumBitMask<eng::math::Direction> transitionNeighbors(const NodeID& nodeInfo) const;

    /*
      \returns The leaf node ID that contains the given index.
               Will return std::nullopt if index is outside of octree bounds.
    */
    std::optional<NodeID> find(const GlobalIndex& index) const;
    std::optional<NodeID> findImpl(const Node& branch, const NodeID& branchInfo, const GlobalIndex& index) const;

    std::vector<NodeID> find(const GlobalBox& region) const;
    void findImpl(std::vector<NodeID>& nodes, const Node& branch, const NodeID& branchInfo, const GlobalBox& region) const;

    std::optional<DrawCommand> createNewMesh(Node& node, const NodeID& nodeID);
    std::optional<DrawCommand> createAdjustedMesh(const Node& node, const NodeID& nodeID) const;

    void createAdjustedMeshesInRegion(std::vector<DrawCommand>& drawCommands, const GlobalBox& region) const;
    void createAdjustedMeshesInRegionImpl(std::vector<DrawCommand>& drawCommands, const Node& branch, const NodeID& branchInfo, const GlobalBox& region) const;

    void checkState() const;
    void checkStateImpl(const Node& node, const NodeID& nodeInfo) const;
  };
}