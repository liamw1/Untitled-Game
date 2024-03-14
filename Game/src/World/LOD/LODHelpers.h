#pragma once
#include "NodeID.h"
#include "Block/Block.h"

namespace lod
{
  struct Vertex
  {
    eng::math::Float3 position;
    eng::math::Float3 isoNormal;
    i32 blockIndex;

    Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, block::Type blockType);
  };

  struct Mesh
  {
    std::vector<u32> indices;
    std::vector<Vertex> vertices;
  };

  struct RenderData
  {
    Mesh primaryMesh;
    eng::EnumArray<Mesh, eng::math::Direction> transitionMeshes;

    uSize totalIndices() const;
    uSize totalVertices() const;
  };

  struct Node
  {
    NodeID id;
    eng::UniqueArray<Node, 8> children;
    std::shared_ptr<RenderData> data;

    Node();
    Node(const NodeID& nodeID);
    bool isLeaf() const;

    bool hasGrandChildren() const;

    void divide();
    void combine();

    static constexpr BlockBox ChildBounds() { return BlockBox(0, 1); }
  };

  class DrawCommand : public eng::IndexedDrawCommand<DrawCommand, NodeID>
  {
    std::vector<u32> m_Indices;
    std::vector<Vertex> m_Vertices;

  public:
    DrawCommand(const NodeID& nodeID, std::vector<u32>&& indices, std::vector<Vertex>&& vertices);

    bool operator==(const DrawCommand& other) const;

    eng::mem::IndexData indexData() const;
    eng::mem::RenderData vertexData() const;
    void clearData();
  };

  struct StateChange
  {
    std::vector<DrawCommand> newDrawCommands;
    std::vector<NodeID> drawCommandsToRemove;

    StateChange(std::vector<DrawCommand>&& _newDrawCommands, std::vector<NodeID>&& _drawCommandsToRemove);
  };
}
