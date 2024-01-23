#include "GMpch.h"
#include "LODHelpers.h"
#include "GlobalParameters.h"

namespace lod
{
  Vertex::Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, block::Type blockType)
    : position(position), isoNormal(isoNormal), textureIndex(static_cast<i32>(blockType.texture(eng::math::Direction::Top))) {}



  Node::Node()
    : Node(NodeID({}, 0)) {}
  Node::Node(const NodeID& nodeID)
    : id(nodeID), children(eng::AllocationPolicy::Deferred) {}

  bool Node::isLeaf() const { return !children; }
  bool Node::hasGrandChildren() const
  {
    return isLeaf() && eng::algo::anyOf(children, [](const Node& child) { return !child.isLeaf(); });
  }

  void Node::divide()
  {
    if (!isLeaf())
      return;
    children.allocate();
    for (const BlockIndex& childIndex : ChildBounds())
      children[ChildBounds().linearIndexOf(childIndex)] = id.child(childIndex);
  }

  void Node::combine()
  {
    if (isLeaf())
      return;
    children.reset();
  }



  DrawCommand::DrawCommand(const NodeID& nodeID, std::vector<u32>&& indices, std::vector<Vertex>&& vertices)
    : eng::IndexedDrawCommand<DrawCommand, NodeID>(nodeID),
      m_Indices(std::move(indices)),
      m_Vertices(std::move(vertices)) {}

  bool DrawCommand::operator==(const DrawCommand& other) const { return id() == other.id(); }

  eng::mem::IndexData DrawCommand::indexData() const { return eng::mem::IndexData(m_Indices); }
  eng::mem::RenderData DrawCommand::vertexData() const { return eng::mem::RenderData(m_Vertices); }
  void DrawCommand::clearData()
  {
    m_Indices = {};
    m_Vertices = {};
  }



  uSize RenderData::totalIndices() const
  {
    uSize indexCount = primaryMesh.indices.size();
    for (const Mesh& transitionMesh : transitionMeshes)
      indexCount += transitionMesh.indices.size();
    return indexCount;
  }

  uSize RenderData::totalVertices() const
  {
    uSize vertexCount = primaryMesh.vertices.size();
    for (const Mesh& transitionMesh : transitionMeshes)
      vertexCount += transitionMesh.vertices.size();
    return vertexCount;
  }

  StateChange::StateChange(std::vector<DrawCommand>&& _newDrawCommands, std::vector<NodeID>&& _drawCommandsToRemove)
    : newDrawCommands(std::move(_newDrawCommands)), drawCommandsToRemove(std::move(_drawCommandsToRemove)) {}
}