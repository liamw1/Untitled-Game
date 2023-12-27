#include "GMpch.h"
#include "LODManager.h"
#include "Player/Player.h"
#include "Util/TransVoxel.h"
#include "World/Chunk/Chunk.h"

namespace newLod
{
  static constexpr u64 c_RootNodeSize = eng::math::pow2<u64>(param::MaxNodeDepth());
  static constexpr GlobalIndex c_RootNodeAnchor = -eng::arithmeticCast<globalIndex_t>(c_RootNodeSize / 2) * GlobalIndex(1);
  static constexpr GlobalBox c_OctreeBounds = GlobalBox(c_RootNodeAnchor, -c_RootNodeAnchor - 1);
  static constexpr NodeID c_RootNodeID = NodeID(c_RootNodeAnchor, 0);

  static constexpr BlockBox c_LODCellBounds(0, param::LODNumCells() - 1);
  static constexpr BlockBox c_LODSampleBounds(0, param::LODNumCells());
  static constexpr f32 c_TransitionCellFractionalWidth = 0.5f;

  // Rendering
  static constexpr i32 c_SSBOBinding = 2;
  static constexpr u32 c_SSBOSize = eng::math::pow2<u32>(20);
  static std::unique_ptr<eng::Shader> s_Shader;
  static std::unique_ptr<eng::ShaderBufferStorage> s_SSBO;
  static const eng::mem::BufferLayout s_VertexBufferLayout = { { eng::mem::DataType::Float3, "a_Position"       },
                                                               { eng::mem::DataType::Float3, "a_IsoNormal"      },
                                                               { eng::mem::DataType::Int2,   "a_TextureIndices" },
                                                               { eng::mem::DataType::Float2, "a_TextureWeighs"  } };

  struct NoiseData
  {
    eng::math::Vec3 position;
    eng::math::Vec3 normal;

    terrain::CompoundSurfaceData surfaceData;
  };

  // LOD smoothness parameter, must be in the range [0.0, 1.0]
  static constexpr f32 smoothnessLevel(i32 lodLevel)
  {
    return 1.0f;
    // return std::min(0.15f * lodLevel + 0.3f, 1.0f);
  }

  // Calculate quantity based on values at corners that compose an edge.  The smoothness parameter s is used to interpolate between 
  // roughest iso-surface (vertex is always chosen at edge midpoint) and the smooth iso-surface interpolation used by Paul Bourke.
  template<typename T>
  static T semiSmoothInterpolation(const T& q0, const T& q1, f32 t, f32 s)
  {
    return ((1 - s) / 2 + s * (1 - t)) * q0 + ((1 - s) / 2 + s * t) * q1;
  }

  static constexpr BlockIndex transitionCellFaceIndexToSampleIndex(const BlockIndex& cellIndex, i32 faceIndex, eng::math::Direction face)
  {
    return cellIndex + BlockIndex(0, faceIndex % 3, faceIndex / 3).permute(eng::math::axisOf(face));
  }

  static BlockArrayRect<terrain::CompoundSurfaceData> generateNoise(const NodeID& node)
  {
    length_t cellLength = node.length() / param::LODNumCells();
    eng::math::Vec2 lodAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node.anchor());

    BlockArrayRect<terrain::CompoundSurfaceData> noiseValues(static_cast<BlockRect>(c_LODSampleBounds), eng::AllocationPolicy::ForOverwrite);
    noiseValues.forEach([cellLength, lodAnchorXY](BlockIndex2D index, terrain::CompoundSurfaceData& surfaceData)
    {
      // Sample noise at cell corners
      eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(index);
      surfaceData = terrain::getSurfaceInfo(pointXY);
    });
    return noiseValues;
  }

  static bool needsMesh(const NodeID& node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t lodFloor = node.anchor().k * Chunk::Length();
    length_t lodCeiling = lodFloor + node.length();

    // Check if LOD is fully below or above surface, if so, no need to generate mesh
    return noiseValues.anyOf([lodFloor, lodCeiling](const terrain::CompoundSurfaceData& surfaceData)
    {
      length_t terrainHeight = surfaceData.getElevation();
      return lodFloor <= terrainHeight && terrainHeight <= lodCeiling;
    });
  }

  static BlockArrayRect<eng::math::Vec3> calculateNoiseNormals(const NodeID& node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t cellLength = node.length() / param::LODNumCells();
    eng::math::Vec2 lodAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node.anchor());

    // Calculate normals using central differences
    BlockArrayRect<eng::math::Vec3> noiseNormals(static_cast<BlockRect>(c_LODSampleBounds), eng::AllocationPolicy::ForOverwrite);
    noiseNormals.forEach([cellLength, lodAnchorXY, &noiseValues](BlockIndex2D index, eng::math::Vec3& normal)
    {
      // Surface heights in adjacent positions.  L - lower, C - center, U - upper
      length_t fLC = 0_m, fUC = 0_m, fCL = 0_m, fCU = 0_m;

      // TODO: Replace with new elevation system
      BlockIndex2D indexLC = index + BlockIndex2D(-1, 0);
      if (index.i == 0)
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexLC);
        fLC = terrain::getElevation(pointXY);
      }
      else
        fLC = noiseValues(indexLC).getElevation();

      BlockIndex2D indexUC = index + BlockIndex2D(1, 0);
      if (index.i == param::LODNumCells())
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexUC);
        fUC = terrain::getElevation(pointXY);
      }
      else
        fUC = noiseValues(indexUC).getElevation();

      BlockIndex2D indexCL = index + BlockIndex2D(0, -1);
      if (index.j == 0)
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexCL);
        fCL = terrain::getElevation(pointXY);
      }
      else
        fCL = noiseValues(indexCL).getElevation();

      BlockIndex2D indexCU = index + BlockIndex2D(0, 1);
      if (index.j == param::LODNumCells())
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexCU);
        fCU = terrain::getElevation(pointXY);
      }
      else
        fCU = noiseValues(indexCU).getElevation();

      eng::math::Vec2 gradient{};
      gradient.x = (fUC - fLC) / (2 * cellLength);
      gradient.y = (fCU - fCL) / (2 * cellLength);

      normal = glm::normalize(eng::math::Vec3(-gradient, 1));
    });
    return noiseNormals;
  }

  static NoiseData interpolateNoiseData(const NodeID& node,
                                        const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues,
                                        const BlockArrayRect<eng::math::Vec3>& noiseNormals,
                                        const BlockIndex& cornerA, const BlockIndex& cornerB, f32 smoothness)
  {
    length_t lodFloor = node.anchor().k * Chunk::Length();
    length_t cellLength = node.length() / param::LODNumCells();

    // Vertex positions
    eng::math::Vec3 posA = static_cast<eng::math::Vec3>(cornerA) * cellLength;
    eng::math::Vec3 posB = static_cast<eng::math::Vec3>(cornerB) * cellLength;

    length_t zA = lodFloor + cornerA.k * cellLength;
    length_t zB = lodFloor + cornerB.k * cellLength;

    const terrain::CompoundSurfaceData& surfaceDataA = noiseValues(static_cast<BlockIndex2D>(cornerA));
    const terrain::CompoundSurfaceData& surfaceDataB = noiseValues(static_cast<BlockIndex2D>(cornerB));

    // Isovalues of corners A and B
    length_t tA = surfaceDataA.getElevation() - zA;
    length_t tB = surfaceDataB.getElevation() - zB;

    // Fraction of distance along edge vertex should be placed
    f32 t = eng::arithmeticCastUnchecked<f32>(tA / (tA - tB));

    eng::math::Vec3 vertexPosition = semiSmoothInterpolation(posA, posB, t, smoothness);
    terrain::CompoundSurfaceData surfaceData = semiSmoothInterpolation(surfaceDataA, surfaceDataB, t, smoothness);

    // Estimate isosurface normal using linear interpolation between corners
    const eng::math::Vec3& n0 = noiseNormals(static_cast<BlockIndex2D>(cornerA));
    const eng::math::Vec3& n1 = noiseNormals(static_cast<BlockIndex2D>(cornerB));
    eng::math::Vec3 isoNormal = semiSmoothInterpolation(n0, n1, t, smoothness);

    return { vertexPosition, isoNormal, surfaceData };
  }

  // Formulas can be found in section 4.4 of TransVoxel paper
  static bool isVertexNearFace(bool isUpstream, f32 u, f32 cellLength)
  {
    return isUpstream ? u > cellLength * (param::LODNumCells() - 1) : u < cellLength;
  }
  static f32 vertexAdjustment1D(bool isUpstream, f32 u, f32 cellLength)
  {
    return c_TransitionCellFractionalWidth * (isUpstream ? ((param::LODNumCells() - 1) * cellLength - u) : (cellLength - u));
  }
  static eng::math::FMat3 calculateVertexTransform(const eng::math::Float3& n)
  {
    return eng::math::FMat3(1 - n.x * n.x, -n.x * n.y, -n.x * n.z,
                            -n.x * n.y, 1 - n.y * n.y, -n.y * n.z,
                            -n.x * n.z, -n.y * n.z, 1 - n.z * n.z);
  }

  static Vertex adjustedPrimaryVertex(const Vertex& vertex, const NodeID& node, eng::EnumBitMask<eng::math::Direction> transitionFaces)
  {
    f32 cellLength = node.length() / param::LODNumCells();

    eng::math::Float3 vertexAdjustment(0.0f);
    bool nearSameResolutionLOD = eng::algo::anyOf(eng::math::Directions(), [cellLength, &vertexAdjustment, &vertex, &transitionFaces](eng::math::Direction face)
    {
      i32 axisID = eng::enumIndex(eng::math::axisOf(face));
      if (!isVertexNearFace(eng::math::isUpstream(face), vertex.position[axisID], cellLength))
        return false;
      if (!transitionFaces[face])
        return true;

      vertexAdjustment[axisID] = vertexAdjustment1D(eng::math::isUpstream(face), vertex.position[axisID], cellLength);
      return false;
    });

    if (nearSameResolutionLOD || vertexAdjustment == eng::math::Float3(0.0f))
      return vertex;

    const eng::math::Float3& normal = vertex.isoNormal;
    eng::math::FMat3 transform = calculateVertexTransform(normal);

    Vertex adjustedVertex = vertex;
    adjustedVertex.position += transform * vertexAdjustment;
    return adjustedVertex;
  }

  static Vertex adjustedTransitionVertex(const Vertex& vertex, const NodeID& node, eng::EnumBitMask<eng::math::Direction> transitionFaces, eng::math::Direction face)
  {
    Vertex adjustedVertex = vertex;

    // If Vertex is on low-resolution side, no adjustment needed.  If on high-resolution side, move vertex to LOD face
    i32 axisID = eng::enumIndex(eng::math::axisOf(face));
    static constexpr f32 tolerance = 128 * std::numeric_limits<f32>::epsilon();
    if (vertex.position[axisID] < tolerance * node.length() || vertex.position[axisID] > (1.0f - tolerance) * node.length())
      return adjustedVertex;

    adjustedVertex.position[axisID] = eng::math::isUpstream(face) ? node.length() : 0.0f;
    return adjustedPrimaryVertex(adjustedVertex, node, transitionFaces);
  }

  static Mesh generatePrimaryMesh(const NodeID& node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues, const BlockArrayRect<eng::math::Vec3>& noiseNormals)
  {
    ENG_PROFILE_FUNCTION();

    struct VertexReuseData
    {
      u32 baseMeshIndex = 0;
      std::array<i8, 4> vertexOrder = { -1, -1, -1, -1 };
    };
    using VertexLayer = std::array<std::array<VertexReuseData, param::LODNumCells()>, param::LODNumCells()>;

    length_t cellLength = node.length() / param::LODNumCells();
    f32 smoothness = smoothnessLevel(node.lodLevel());

    Mesh primaryMesh;
    VertexLayer prevLayer{};
    VertexLayer currLayer{};
    eng::algo::forEach(c_LODCellBounds, [cellLength, smoothness, &primaryMesh, &prevLayer, &currLayer, &node, &noiseValues, &noiseNormals](const BlockIndex& cellIndex)
    {
      // If j and k are 0, we are starting a new layer of cells
      if (cellIndex.j == 0 && cellIndex.k == 0)
      {
        prevLayer = std::move(currLayer);
        currLayer = {};
      }

      // Determine which of the 256 cases the cell belongs to
      u8 cellCase = 0;
      for (i32 cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
      {
        BlockIndex sampleIndex = cellIndex + BlockIndex(cornerIndex % 2, (cornerIndex / 2) % 2, cornerIndex / 4);
        length_t sampleZ = node.anchor().k * Chunk::Length() + sampleIndex.k * cellLength;
        if (noiseValues(static_cast<BlockIndex2D>(sampleIndex)).getElevation() > sampleZ)
          cellCase |= eng::bit(cornerIndex);
      }
      if (cellCase == 0 || cellCase == 255) // These cases don't have triangles
        return;

      currLayer[cellIndex.j][cellIndex.k].baseMeshIndex = eng::arithmeticCastUnchecked<u32>(primaryMesh.vertices.size());

      // Use lookup table to determine which of 15 equivalence classes the cell belongs to
      u8 cellEquivalenceClass = c_RegularCellClass[cellCase];
      RegularCellData cellData = c_RegularCellData[cellEquivalenceClass];
      i32 triangleCount = cellData.getTriangleCount();

      // Loop over all triangles in cell
      i32 cellVertexCount = 0;
      std::array<u32, c_MaxCellVertexCount> prevCellVertexIndices{};
      for (i32 vert = 0; vert < 3 * triangleCount; ++vert)
      {
        i32 edgeIndex = cellData.vertexIndex[vert];

        // Check if vertex has already been created in this cell
        i32 vertexIndex = prevCellVertexIndices[edgeIndex];
        if (vertexIndex > 0)
        {
          primaryMesh.indices.push_back(vertexIndex);
          continue;
        }

        // Lookup placement of corners A,B that form the cell edge new vertex lies on
        u16 vertexData = c_RegularVertexData[cellCase][edgeIndex];
        u8 sharedVertexReuseIndex = (vertexData >> 8) & 0xF;
        u8 sharedVertexDirectionNibble = (vertexData >> 12) & 0xF;
        bool newVertex = sharedVertexDirectionNibble == 8;

        // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
        if (newVertex)
          currLayer[cellIndex.j][cellIndex.k].vertexOrder[sharedVertexReuseIndex] = cellVertexCount;
        else
        {
          BlockIndex sharedVertexDirection = -BlockIndex(eng::EnumBitMask<eng::math::Axis>(sharedVertexDirectionNibble));
          BlockIndex sharedVertexIndex = cellIndex + sharedVertexDirection;

          if (sharedVertexIndex.nonNegative())
          {
            const VertexLayer& targetLayer = sharedVertexIndex.i == cellIndex.i ? currLayer : prevLayer;

            i32 baseMeshIndex = targetLayer[sharedVertexIndex.j][sharedVertexIndex.k].baseMeshIndex;
            i32 vertexOrder = targetLayer[sharedVertexIndex.j][sharedVertexIndex.k].vertexOrder[sharedVertexReuseIndex];
            if (baseMeshIndex > 0 && vertexOrder >= 0)
            {
              primaryMesh.indices.push_back(baseMeshIndex + vertexOrder);
              prevCellVertexIndices[edgeIndex] = baseMeshIndex + vertexOrder;
              continue;
            }
          }
        }

        // Extract the local corner indices from vertex data
        u8 cornerNibbleA = static_cast<u8>((vertexData >> 0) & 0xF);
        u8 cornerNibbleB = static_cast<u8>((vertexData >> 4) & 0xF);

        BlockIndex cornerIndexA = cellIndex + BlockIndex(eng::EnumBitMask<eng::math::Axis>(cornerNibbleA));
        BlockIndex cornerIndexB = cellIndex + BlockIndex(eng::EnumBitMask<eng::math::Axis>(cornerNibbleB));

        NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, cornerIndexA, cornerIndexB, smoothness);

        u32 vertexCount = eng::arithmeticCastUnchecked<u32>(primaryMesh.vertices.size());
        primaryMesh.vertices.emplace_back(noiseData.position, noiseData.normal, noiseData.surfaceData.getTextureIndices(), noiseData.surfaceData.getTextureWeights());
        primaryMesh.indices.push_back(vertexCount);
        prevCellVertexIndices[edgeIndex] = vertexCount;

        cellVertexCount++;
      }
    });
    return primaryMesh;
  }

  static Mesh generateTransitionMesh(const NodeID& node,
                                     const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues,
                                     const BlockArrayRect<eng::math::Vec3>& noiseNormals,
                                     eng::math::Direction face)
  {
    ENG_PROFILE_FUNCTION();

    static constexpr i32 numLowResultionCells = param::LODNumCells() / 2;
    static constexpr BlockRect lowResolutionCellBounds(0, numLowResultionCells - 1);

    struct VertexReuseData
    {
      u32 baseMeshIndex = 0;
      std::array<i8, 10> vertexOrder = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    };
    using VertexStrip = std::array<VertexReuseData, numLowResultionCells>;

    length_t cellLength = node.length() / param::LODNumCells();

    // Generate transition mesh using Transvoxel algorithm
    Mesh transitionMesh;
    VertexStrip prevStrip{};
    VertexStrip currStrip{};
    eng::algo::forEach(lowResolutionCellBounds, [cellLength, &transitionMesh, &prevStrip, &currStrip, &node, &noiseValues, &noiseNormals, face](BlockIndex2D lowResolutionCellIndex)
    {
      // If j is 0, we are starting a new strip of cells
      if (lowResolutionCellIndex.j == 0)
      {
        prevStrip = std::move(currStrip);
        currStrip = {};
      }

      BlockIndex cellIndex = BlockIndex(c_LODSampleBounds.limitAlongDirection(face), 2 * lowResolutionCellIndex).permute(eng::math::axisOf(face));

      // Determine which of the 512 cases the cell belongs to
      u16 cellCase = 0;
      for (i32 transitionCellFaceIndex = 0; transitionCellFaceIndex < 9; ++transitionCellFaceIndex)
      {
        BlockIndex sampleIndex = transitionCellFaceIndexToSampleIndex(cellIndex, transitionCellFaceIndex, face);
        length_t sampleZ = node.anchor().k * Chunk::Length() + sampleIndex.k * cellLength;
        if (noiseValues[sampleIndex.i][sampleIndex.j].getElevation() > sampleZ)
          cellCase |= eng::bit(c_TransitionCellFaceIndexToBitFlip[transitionCellFaceIndex]);
      }
      if (cellCase == 0 || cellCase == 511) // These cases don't have triangles
        return;

      currStrip[lowResolutionCellIndex.j].baseMeshIndex = eng::arithmeticCastUnchecked<u32>(transitionMesh.vertices.size());

      // Use lookup table to determine which of 56 equivalence classes the cell belongs to
      u8 cellEquivClass = c_TransitionCellClass[cellCase];
      bool reverseWindingOrder = cellEquivClass >> 7;
      TransitionCellData cellData = c_TransitionCellData[cellEquivClass & 0x7F];
      i32 triangleCount = cellData.getTriangleCount();

      // Loop over all triangles in cell
      i32 cellVertexCount = 0;
      std::array<u32, c_MaxCellVertexCount> prevCellVertexIndices{};
      for (i32 vert = 0; vert < 3 * triangleCount; ++vert)
      {
        i32 edgeIndex = cellData.vertexIndex[reverseWindingOrder ? 3 * triangleCount - 1 - vert : vert];

        // Check if vertex has already been created in this cell
        i32 vertexIndex = prevCellVertexIndices[edgeIndex];
        if (vertexIndex > 0)
        {
          transitionMesh.indices.push_back(vertexIndex);
          continue;
        }

        // Lookup indices of vertices A,B of the cell edge that vertex v lies on
        u16 vertexData = c_TransitionVertexData[cellCase][edgeIndex];
        u8 sharedVertexReuseIndex = (vertexData >> 8) & 0xF;
        u8 sharedVertexDirectionNibble = (vertexData >> 12) & 0xF;
        bool isReusable = sharedVertexDirectionNibble != 4;
        bool newVertex = sharedVertexDirectionNibble == 8;

        // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
        if (newVertex)
          currStrip[lowResolutionCellIndex.j].vertexOrder[sharedVertexReuseIndex] = cellVertexCount;
        else if (isReusable)
        {
          BlockIndex2D sharedVertexDirection = -BlockIndex2D(eng::EnumBitMask<eng::math::Axis>(sharedVertexDirectionNibble));
          BlockIndex2D sharedVertexIndex = lowResolutionCellIndex + sharedVertexDirection;

          if (sharedVertexIndex.nonNegative())
          {
            const auto& targetStrip = sharedVertexIndex.i == lowResolutionCellIndex.i ? currStrip : prevStrip;

            i32 baseMeshIndex = targetStrip[sharedVertexIndex.j].baseMeshIndex;
            i32 vertexOrder = targetStrip[sharedVertexIndex.j].vertexOrder[sharedVertexReuseIndex];
            if (baseMeshIndex > 0 && vertexOrder >= 0)
            {
              transitionMesh.indices.push_back(baseMeshIndex + vertexOrder);
              prevCellVertexIndices[edgeIndex] = baseMeshIndex + vertexOrder;
              continue;
            }
          }
        }

        // Extract the local transition cell sample indices from vertex data
        u8 sampleNibbleA = (vertexData >> 0) & 0xF;
        u8 sampleNibbleB = (vertexData >> 4) & 0xF;
        bool isOnLowResSide = sampleNibbleB > 8;

        // Indices of samples A,B
        BlockIndex sampleIndexA = transitionCellFaceIndexToSampleIndex(cellIndex, c_TransitionCellSampleIndexToTransitionCellFaceIndex[sampleNibbleA], face);
        BlockIndex sampleIndexB = transitionCellFaceIndexToSampleIndex(cellIndex, c_TransitionCellSampleIndexToTransitionCellFaceIndex[sampleNibbleB], face);

        // If vertex is on low-resolution side, use smoothness level of low-resolution LOD
        f32 smoothness = smoothnessLevel(node.lodLevel() + isOnLowResSide);
        NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, sampleIndexA, sampleIndexB, smoothness);

        if (!isOnLowResSide)
          noiseData.position -= c_TransitionCellFractionalWidth * cellLength * static_cast<eng::math::Vec3>(BlockIndex::Dir(face));

        u32 vertexCount = eng::arithmeticCastUnchecked<u32>(transitionMesh.vertices.size());
        transitionMesh.vertices.emplace_back(noiseData.position, noiseData.normal, noiseData.surfaceData.getTextureIndices(), noiseData.surfaceData.getTextureWeights());
        transitionMesh.indices.push_back(vertexCount);
        prevCellVertexIndices[edgeIndex] = vertexCount;

        cellVertexCount++;
      }
    });

    // If transition mesh is on downstream side, we must reverse triangle orientation
    if (!isUpstream(face))
      eng::algo::reverse(transitionMesh.indices);

    return transitionMesh;
  }

  static std::shared_ptr<RenderData> generateRenderData(const NodeID& node)
  {
    ENG_PROFILE_FUNCTION();

    // Generate voxel data using heightmap
    BlockArrayRect<terrain::CompoundSurfaceData> noiseValues = generateNoise(node);

    if (!needsMesh(node, noiseValues))
      return nullptr;

    // Generate normal data from heightmap
    BlockArrayRect<eng::math::Vec3> noiseNormals = calculateNoiseNormals(node, noiseValues);

    std::shared_ptr<RenderData> renderData = std::make_shared<RenderData>();
    renderData->primaryMesh = generatePrimaryMesh(node, noiseValues, noiseNormals);
    for (eng::math::Direction face : eng::math::Directions())
      renderData->transitionMeshes[face] = generateTransitionMesh(node, noiseValues, noiseNormals, face);
    return renderData;
  }

  static DrawCommand createDrawCommand(const RenderData& renderData, const NodeID& node, eng::EnumBitMask<eng::math::Direction> transitionFaces)
  {
    Mesh fullMesh;
    fullMesh.indices.reserve(renderData.totalIndices());
    fullMesh.vertices.reserve(renderData.totalVertices());

    // Add adjusted primary mesh
    for (u32 index : renderData.primaryMesh.indices)
      fullMesh.indices.push_back(index);
    for (const Vertex& vertex : renderData.primaryMesh.vertices)
      fullMesh.vertices.push_back(adjustedPrimaryVertex(vertex, node, transitionFaces));

    // Add adjusted transition mesh if it exists
    for (eng::math::Direction face : eng::math::Directions())
      if (transitionFaces[face])
      {
        u32 baseIndex = eng::arithmeticCast<u32>(fullMesh.vertices.size());

        for (u32 index : renderData.transitionMeshes[face].indices)
          fullMesh.indices.push_back(baseIndex + index);
        for (const Vertex& vertex : renderData.transitionMeshes[face].vertices)
          fullMesh.vertices.push_back(adjustedTransitionVertex(vertex, node, transitionFaces, face));
      }

    return DrawCommand(node, std::move(fullMesh.indices), std::move(fullMesh.vertices));
  }



  LODManager::LODManager()
    : m_MultiDrawArray(s_VertexBufferLayout),
      m_ThreadPool("LOD Manager", 1)
  {
    ENG_PROFILE_FUNCTION();

    s_Shader = eng::Shader::Create("assets/shaders/LOD.glsl");
    s_SSBO = std::make_unique<eng::ShaderBufferStorage>(c_SSBOBinding, c_SSBOSize);
  }

  LODManager::~LODManager()
  {
    m_ThreadPool.shutdown();
  }

  void LODManager::render()
  {
    ENG_PROFILE_FUNCTION();

    eng::math::Mat4 viewProjection = eng::scene::CalculateViewProjection(eng::scene::ActiveCamera());
    eng::EnumArray<eng::math::Vec4, eng::math::FrustumPlane> frustumPlanes = eng::math::calculateViewFrustumPlanes(viewProjection);
    eng::EnumArray<eng::math::Vec4, eng::math::FrustumPlane> shiftedFrustumPlanes = frustumPlanes;
    eng::EnumArray<length_t, eng::math::FrustumPlane> planeNormalMagnitudes;

    for (eng::math::FrustumPlane plane : eng::math::FrustumPlanes())
      planeNormalMagnitudes[plane] = glm::length(static_cast<eng::math::Vec3>(frustumPlanes[plane]));

    GlobalIndex originIndex = player::originIndex();

    s_Shader->bind();

    eng::render::command::setFaceCulling(true);
    eng::render::command::setDepthWriting(true);
    eng::render::command::setUseDepthOffset(false);
    uSize commandCount = m_MultiDrawArray.partition([this, &frustumPlanes, &shiftedFrustumPlanes, &planeNormalMagnitudes, &originIndex](const NodeID& nodeID)
    {
      // Shift each plane by distance equal to radius of sphere that circumscribes LOD
      length_t LODSphereRadius = nodeID.boundingSphereRadius();
      for (eng::math::FrustumPlane plane : eng::math::FrustumPlanes())
        shiftedFrustumPlanes[plane].w = frustumPlanes[plane].w + LODSphereRadius * planeNormalMagnitudes[plane];

      return eng::math::isInFrustum(nodeID.center(originIndex), shiftedFrustumPlanes);
    });

    std::vector<eng::math::Float4> storageBufferData;
    storageBufferData.reserve(commandCount);
    const std::vector<DrawCommand>& drawCommands = m_MultiDrawArray.getDrawCommandBuffer();
    for (uSize i = 0; i < commandCount; ++i)
    {
      eng::math::Vec3 nodeAnchorPosition = drawCommands[i].id().anchorPosition(originIndex);
      storageBufferData.emplace_back(nodeAnchorPosition, 0);
    }
    s_SSBO->write(storageBufferData);

    m_MultiDrawArray.bind();
    block::bindAverageColorSSBO();
    eng::render::command::multiDrawIndexed(drawCommands, commandCount);
  }

  void LODManager::update()
  {
    ENG_PROFILE_FUNCTION();

    if (!m_UpdateFuture.valid() || eng::thread::isReady(m_UpdateFuture))
      m_UpdateFuture = m_ThreadPool.submit(eng::thread::Priority::Low, &LODManager::updateTask, this);

    /*
      We may want to limit the number of state changes per function call to reduce lag spikes.
      We can also consider combining state changes to reduce redundant changes in state.
    */
    while (std::optional<StateChange> stateChange = m_StateChangeQueue.tryPop())
    {
      for (DrawCommand& drawCommand : stateChange->newDrawCommands)
        m_MultiDrawArray.insert(std::move(drawCommand));
      for (const NodeID& id : stateChange->drawCommandsToRemove)
        m_MultiDrawArray.remove(id);
    }
  }

  void LODManager::updateTask()
  {
    ENG_PROFILE_FUNCTION();

    while (updateRecursively(m_Root, c_RootNodeID, player::originIndex()))
    {
    }
  }

  bool LODManager::updateRecursively(Node& branch, const NodeID& branchInfo, const GlobalIndex& originIndex)
  {
    bool stateChanged = false;
    if (branchInfo.lodLevel() < 1)
      return stateChanged;

    if (branch.isLeaf())
      stateChanged |= tryDivide(branch, branchInfo, originIndex);
    else if (!branch.hasGrandChildren())
      stateChanged |= tryCombine(branch, branchInfo, originIndex);

    if (branch.isLeaf())
      return stateChanged;

    branch.children.forEach([this, &branchInfo, &originIndex, &stateChanged](const BlockIndex& childIndex, Node& child)
    {
      stateChanged |= updateRecursively(child, branchInfo.child(childIndex), originIndex);
    });
    return stateChanged;
  }

  bool LODManager::tryDivide(Node& node, const NodeID& nodeInfo, const GlobalIndex& originIndex)
  {
    ENG_PROFILE_FUNCTION();

    i32 dividedLodLevel = nodeInfo.lodLevel() - 1;
    for (const std::optional<NodeID>& neighbor : neighborQuery(nodeInfo))
      if (neighbor && std::abs(dividedLodLevel - neighbor->lodLevel()) > 1)
        return false;

    globalIndex_t splitRange = 2 * nodeInfo.size() - 1 + param::RenderDistance();
    GlobalBox splitRangeBoundingBox(originIndex - splitRange, originIndex + splitRange);
    if (!splitRangeBoundingBox.overlapsWith(nodeInfo.boundingBox()))
      return false;

    std::vector<DrawCommand> newDrawCommands;
    std::vector<NodeID> removedNodes = { nodeInfo };

    node.divide();
    node.children.forEach([this, &nodeInfo, &newDrawCommands](const BlockIndex& childIndex, Node& child)
    {
      std::optional<DrawCommand> drawCommand = createNewMesh(child, nodeInfo.child(childIndex));
      if (drawCommand)
        newDrawCommands.push_back(std::move(*drawCommand));
    });

    for (const GlobalBox& adjustmentRegion : eng::math::FaceInteriors(nodeInfo.boundingBox().expand()))
      createAdjustedMeshesInRegion(newDrawCommands, adjustmentRegion);

    m_StateChangeQueue.emplace(std::move(newDrawCommands), std::move(removedNodes));
    return true;
  }

  bool LODManager::tryCombine(Node& parentNode, const NodeID& nodeInfo, const GlobalIndex& originIndex)
  {
    ENG_PROFILE_FUNCTION();

    for (const std::optional<NodeID>& neighbor : neighborQuery(nodeInfo))
      if (neighbor && std::abs(nodeInfo.lodLevel() - neighbor->lodLevel()) > 1)
        return false;

    for (const GlobalBox& faceNeighborRegion : eng::math::FaceInteriors(nodeInfo.boundingBox().expand()))
    {
      std::vector<NodeID> faceNeighbors = find(faceNeighborRegion);
      for (const NodeID& neighbor : faceNeighbors)
        if (neighbor.lodLevel() != faceNeighbors.front().lodLevel())
          return false;
    }

    globalIndex_t combineRange = 2 * nodeInfo.size() - 1 + param::RenderDistance();
    GlobalBox combineRangeBoundingBox(originIndex - combineRange, originIndex + combineRange);
    if (combineRangeBoundingBox.overlapsWith(nodeInfo.boundingBox()))
      return false;

    std::vector<DrawCommand> newDrawCommands;
    std::vector<NodeID> removedNodes;

    parentNode.combine();
    eng::algo::forEach(parentNode.children.bounds(), [&nodeInfo, &removedNodes](const BlockIndex& childIndex)
    {
      removedNodes.push_back(nodeInfo.child(childIndex));
    });

    std::optional<DrawCommand> drawCommand = createNewMesh(parentNode, nodeInfo);
    if (drawCommand)
      newDrawCommands.push_back(std::move(*drawCommand));

    for (const GlobalBox& adjustmentRegion : eng::math::FaceInteriors(nodeInfo.boundingBox().expand()))
      createAdjustedMeshesInRegion(newDrawCommands, adjustmentRegion);

    m_StateChangeQueue.emplace(std::move(newDrawCommands), std::move(removedNodes));
    return true;
  }

  eng::EnumArray<std::optional<NodeID>, eng::math::Direction> LODManager::neighborQuery(const NodeID& nodeInfo) const
  {
    eng::EnumArray<std::optional<NodeID>, eng::math::Direction> neighbors;
    for (eng::math::Direction face : eng::math::Directions())
    {
      GlobalIndex offset = (eng::math::isUpstream(face) ? nodeInfo.size() : 1) * GlobalIndex::Dir(face);
      neighbors[face] = find(nodeInfo.anchor() + offset);
    }
    return neighbors;
  }

  eng::EnumBitMask<eng::math::Direction> LODManager::transitionNeighbors(const NodeID& nodeInfo) const
  {
    eng::EnumArray<std::optional<NodeID>, eng::math::Direction> neighborQueryResult = neighborQuery(nodeInfo);

    eng::EnumBitMask<eng::math::Direction> transitionNeighbors;
    for (eng::math::Direction face : eng::math::Directions())
    {
      const std::optional<NodeID>& neighbor = neighborQueryResult[face];
      if (!neighbor)
        continue;

      if (neighbor->lodLevel() - nodeInfo.lodLevel() == 1)
        transitionNeighbors.set(face);
      else if (neighbor->lodLevel() - nodeInfo.lodLevel() > 1)
        ENG_WARN("LOD neighbor is more than one level lower resolution!");
    }
    return transitionNeighbors;
  }

  std::optional<NodeID> LODManager::find(const GlobalIndex& index) const
  {
    if (!c_OctreeBounds.encloses(index))
      return std::nullopt;
    return findImpl(m_Root, c_RootNodeID, index);
  }

  std::optional<NodeID> LODManager::findImpl(const Node& branch, const NodeID& branchInfo, const GlobalIndex& index) const
  {
    if (branch.isLeaf())
      return branchInfo;

    BlockIndex childIndex = branchInfo.childIndex(index);
    return findImpl(branch.children(childIndex), branchInfo.child(childIndex), index);
  }

  std::vector<NodeID> LODManager::find(const GlobalBox& region) const
  {
    std::vector<NodeID> foundNodes;
    findImpl(foundNodes, m_Root, c_RootNodeID, region);
    return foundNodes;
  }

  void LODManager::findImpl(std::vector<NodeID>& nodes, const Node& branch, const NodeID& branchInfo, const GlobalBox& region) const
  {
    if (!region.overlapsWith(branchInfo.boundingBox()))
      return;

    if (branch.isLeaf())
      nodes.push_back(branchInfo);
    else
      branch.children.forEach([this, &nodes, &branchInfo, &region](const BlockIndex& childIndex, const Node& child)
      {
        findImpl(nodes, child, branchInfo.child(childIndex), region);
      });
  }

  std::optional<DrawCommand> LODManager::createNewMesh(Node& node, const NodeID& nodeInfo)
  {
    if (nodeInfo.lodLevel() > param::HighestRenderableLODLevel())
      return std::nullopt;

    node.data = generateRenderData(nodeInfo);
    if (!node.data)
      return std::nullopt;

    eng::EnumBitMask<eng::math::Direction> transitionFaces = transitionNeighbors(nodeInfo);
    return createDrawCommand(*node.data, nodeInfo, transitionFaces);
  }

  std::optional<DrawCommand> LODManager::createAdjustedMesh(const Node& node, const NodeID& nodeID) const
  {
    if (!node.data)
      return std::nullopt;

    eng::EnumBitMask<eng::math::Direction> transitionFaces = transitionNeighbors(nodeID);
    return createDrawCommand(*node.data, nodeID, transitionFaces);
  }

  void LODManager::createAdjustedMeshesInRegion(std::vector<DrawCommand>& drawCommands, const GlobalBox& region) const
  {
    createAdjustedMeshesInRegionImpl(drawCommands, m_Root, c_RootNodeID, region);
  }

  void LODManager::createAdjustedMeshesInRegionImpl(std::vector<DrawCommand>& drawCommands, const Node& branch, const NodeID& branchInfo, const GlobalBox& region) const
  {
    if (!region.overlapsWith(branchInfo.boundingBox()))
      return;

    if (branch.isLeaf())
    {
      if (std::optional<DrawCommand> drawCommand = createAdjustedMesh(branch, branchInfo))
        drawCommands.push_back(std::move(*drawCommand));
    }
    else
      branch.children.forEach([this, &drawCommands, &branchInfo, &region](const BlockIndex& childIndex, const Node& child)
      {
        createAdjustedMeshesInRegionImpl(drawCommands, child, branchInfo.child(childIndex), region);
      });
  }

  void newLod::LODManager::checkState() const
  {
    checkStateImpl(m_Root, c_RootNodeID);
  }

  void LODManager::checkStateImpl(const Node& node, const NodeID& nodeInfo) const
  {
    if (node.isLeaf())
    {
      for (const std::optional<NodeID>& neighbor : neighborQuery(nodeInfo))
        if (neighbor && std::abs(nodeInfo.lodLevel() - neighbor->lodLevel()) > 1)
          ENG_ERROR("LOD tree is in incorrect state!");
      return;
    }

    node.children.forEach([this, &nodeInfo](const BlockIndex& childIndex, const Node& child)
    {
      checkStateImpl(child, nodeInfo.child(childIndex));
    });
  }
}