#pragma once
#include "LODHelpers.h"
#include "World/Terrain.h"

/*
  Level of Detail (LOD) system.  The game world is partitioned with an octree,
  the leaf nodes of which we often refer to as an "LOD".  Each LOD is given an
  "LOD level" based on its depth in the tree.  This LOD level represents the
  level of simplification of the voxel/noise data.  At level 0, LODs have as
  many polygons as chunks, while at high LOD levels individual polygons can
  span multiple chunks.
*/
namespace newLod
{
  class LODManager
  {
    eng::MultiDrawArray<DrawCommand> m_MultiDrawArray;

    // Multi-threading
    eng::thread::Queue<StateChange> m_StateChangeQueue;
    eng::thread::ThreadPool m_ThreadPool;
    std::future<void> m_UpdateFuture;

    // LOD data
    Node m_Root;

  public:
    LODManager();
    ~LODManager();

    void render();

    void update();

  private:
    void updateTask();
    bool updateNew(const GlobalIndex& originIndex);
    bool updateRecursively(Node& node, const GlobalIndex& originIndex);

    bool tryDivide(Node& leafNode, const GlobalIndex& originIndex);
    bool tryCombine(Node& parentNode, const GlobalIndex& originIndex);

    eng::EnumArray<std::optional<NodeID>, eng::math::Direction> neighborQuery(const Node& node) const;
    eng::EnumBitMask<eng::math::Direction> transitionNeighbors(const Node& node) const;

    /*
      \returns The leaf node ID that contains the given index.
               Will return std::nullopt if index is outside of octree bounds.
    */
    std::optional<NodeID> find(const GlobalIndex& index) const;
    std::optional<NodeID> findImpl(const Node& node, const GlobalIndex& index) const;

    std::vector<NodeID> find(const GlobalBox& region) const;
    void findImpl(std::vector<NodeID>& nodes, const Node& node, const GlobalBox& region) const;

    std::optional<DrawCommand> createNewMesh(Node& node);
    std::optional<DrawCommand> createAdjustedMesh(const Node& node) const;

    void createAdjustedMeshesInRegion(std::vector<DrawCommand>& drawCommands, const GlobalBox& region) const;
    void createAdjustedMeshesInRegionImpl(std::vector<DrawCommand>& drawCommands, const Node& node, const GlobalBox& region) const;

    std::vector<Node*> reverseLevelOrder();
    void reverseLevelOrderImpl(std::vector<Node*>& nodes, Node& node);

    void checkState() const;
    void checkStateImpl(const Node& node) const;
  };
}