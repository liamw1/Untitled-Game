#pragma once
#include "NodeID.h"

namespace newLod
{
  struct Vertex
  {
    eng::math::Float3 position;
    eng::math::Float3 isoNormal;
    std::array<i32, 2> textureIndices;
    eng::math::Float2 textureWeights;

    Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, const std::array<i32, 2>& textureIndices, const eng::math::Float2& textureWeights);
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
    BlockArrayBox<Node> children;
    std::shared_ptr<RenderData> data;

    Node();
    bool isLeaf() const;

    void divide();
    void combine();
  };

  class DrawCommand : public eng::IndexedDrawCommand<DrawCommand, NodeID>
  {
    std::vector<u32> m_Indices;
    std::vector<Vertex> m_Vertices;

  public:
    DrawCommand(const NodeID& nodeID, std::vector<u32>&& indices, std::vector<Vertex>&& vertices);

    bool operator==(const DrawCommand& other) const;

    eng::mem::IndexData indexData() const;
    eng::mem::Data vertexData() const;
    void clearData();
  };
}
