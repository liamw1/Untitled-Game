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
    i32 quadIndex;

    Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, const std::array<i32, 2>& textureIndices, const eng::math::Float2& textureWeights, i32 quadIndex);
  };

  class DrawCommand : public eng::IndexedDrawCommand<DrawCommand, MeshID>
  {
    std::vector<u32> m_Indices;
    std::vector<Vertex> m_Vertices;

  public:
    DrawCommand(const MeshID& meshID, std::vector<u32>&& indices, std::vector<Vertex>&& vertices);

    bool operator==(const DrawCommand& other) const;

    u32 vertexCount() const;

    eng::mem::IndexData indexData() const;
    eng::mem::Data vertexData() const;
    void clearData();
  };
}
