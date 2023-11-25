#include "GMpch.h"
#include "LODHelpers.h"

namespace newLod
{
  Vertex::Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, const std::array<i32, 2>& textureIndices, const eng::math::Float2& textureWeights, i32 quadIndex)
    : position(position), isoNormal(isoNormal), textureIndices(textureIndices), textureWeights(textureWeights), quadIndex(quadIndex) {}



  DrawCommand::DrawCommand(const MeshID& meshID, std::vector<u32>&& indices, std::vector<Vertex>&& vertices)
    : eng::IndexedDrawCommand<DrawCommand, MeshID>(meshID),
      m_Indices(std::move(indices)),
      m_Vertices(std::move(vertices)) {}

  bool DrawCommand::operator==(const DrawCommand& other) const { return id() == other.id(); }

  u32 DrawCommand::vertexCount() const { return eng::arithmeticCast<u32>(m_Vertices.size()); }
  eng::mem::IndexData DrawCommand::indexData() const { return eng::mem::IndexData(m_Indices); }
  eng::mem::Data DrawCommand::vertexData() const { return eng::mem::Data(m_Vertices); }
  void DrawCommand::clearData() {}
}