#include "GMpch.h"
#include "LODHelpers.h"
#include "GlobalParameters.h"

namespace newLod
{
  // Formulas can be found in section 4.4 of TransVoxel paper
  static bool isVertexNearFace(bool isUpstream, f32 u, f32 cellLength)
  {
    return isUpstream ? u > cellLength * (param::ChunkSize() - 1) : u < cellLength;
  }
  static f32 vertexAdjustment1D(bool isUpstream, f32 u, f32 cellLength)
  {
    return param::TransitionCellFractionalWidth() * (isUpstream ? ((param::ChunkSize() - 1) * cellLength - u) : (cellLength - u));
  }
  static eng::math::FMat3 calcVertexTransform(const eng::math::Float3& n)
  {
    return eng::math::FMat3(1 - n.x * n.x, -n.x * n.y, -n.x * n.z,
                            -n.x * n.y, 1 - n.y * n.y, -n.y * n.z,
                            -n.x * n.z, -n.y * n.z, 1 - n.z * n.z);
  }

  static void adjustVertex(Vertex& vertex, eng::EnumBitMask<eng::math::Direction> transitionFaces, f32 cellLength)
  {
    eng::math::Float3 vertexAdjustment{};
    bool isNearSameResolutionLOD = false;
    for (eng::math::Direction face : eng::math::Directions())
    {
      i32 axisID = eng::enumIndex(eng::math::axisOf(face));
      if (isVertexNearFace(eng::math::isUpstream(face), vertex.position[axisID], cellLength))
      {
        if (transitionFaces[face])
          vertexAdjustment[axisID] = vertexAdjustment1D(eng::math::isUpstream(face), vertex.position[axisID], cellLength);
        else
        {
          isNearSameResolutionLOD = true;
          break;
        }
      }
    }

    if (!isNearSameResolutionLOD && vertexAdjustment != eng::math::Float3(0.0))
    {
      const eng::math::Float3& n = vertex.isoNormal;
      eng::math::FMat3 transform = calcVertexTransform(n);

      vertex.position += transform * vertexAdjustment;
    }
  }



  Vertex::Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, const std::array<i32, 2>& textureIndices, const eng::math::Float2& textureWeights)
    : position(position), isoNormal(isoNormal), textureIndices(textureIndices), textureWeights(textureWeights) {}



  DrawCommand::DrawCommand(const MeshID& meshID, std::vector<u32>&& indices, std::vector<Vertex>&& vertices)
    : eng::IndexedDrawCommand<DrawCommand, MeshID>(meshID),
      m_Indices(std::move(indices)),
      m_Vertices(std::move(vertices)) {}

  bool DrawCommand::operator==(const DrawCommand& other) const { return id() == other.id(); }

  eng::mem::IndexData DrawCommand::indexData() const { return eng::mem::IndexData(m_Indices); }
  eng::mem::Data DrawCommand::vertexData() const
  {
    if (m_AdjustmentState.empty())
      return eng::mem::Data(m_Vertices);
    else
      return eng::mem::Data(m_AdjustedVertices);
  }
  void DrawCommand::clearData()
  {
    m_Indices = {};
    m_AdjustedVertices = {};
  }

  bool DrawCommand::adjustVertices(eng::EnumBitMask<eng::math::Direction> transitionFaces)
  {
    if (m_AdjustmentState == transitionFaces)
      return false;
    m_AdjustmentState = transitionFaces;

    // Can return early if mesh doesn't have transition faces
    if (m_AdjustmentState.empty())
      return true;

    m_AdjustedVertices = m_Vertices;

    f32 nodeLength = eng::arithmeticCast<f32>(id().nodeID().length());
    f32 cellLength = nodeLength / param::ChunkSize();
    if (id().isPrimaryMesh())
      for (Vertex& vertex : m_AdjustedVertices)
        adjustVertex(vertex, transitionFaces, cellLength);
    else
      for (Vertex& vertex : m_AdjustedVertices)
      {
        eng::math::Direction face = *id().face();
        i32 axisID = eng::enumIndex(eng::math::axisOf(face));

        // If Vertex is on low-resolution side, skip.  If on high-resolution side, move vertex to LOD face
        static constexpr f32 tolerance = 128 * std::numeric_limits<f32>::epsilon();
        if (vertex.position[axisID] < tolerance * nodeLength || vertex.position[axisID] > (1.0f - tolerance) * nodeLength)
          continue;

        vertex.position[axisID] = eng::math::isUpstream(face) ? nodeLength : 0.0f;
        adjustVertex(vertex, transitionFaces, cellLength);
      }

    return true;
  }
}