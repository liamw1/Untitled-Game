#include "GMpch.h"
#include "LODManager.h"
#include "Player/Player.h"
#include "Util/TransVoxel.h"
#include "World/Chunk/Chunk.h"
#include "World/Terrain.h"

namespace lod
{
  static constexpr u64 c_RootNodeSize = eng::math::pow2<u64>(param::MaxNodeDepth());
  static constexpr GlobalIndex c_RootNodeAnchor = -eng::arithmeticCast<globalIndex_t>(c_RootNodeSize / 2) * GlobalIndex(1);
  static constexpr GlobalBox c_OctreeBounds = GlobalBox(c_RootNodeAnchor, -c_RootNodeAnchor - 1);
  static constexpr NodeID c_RootNodeID = NodeID(c_RootNodeAnchor, 0);

  static constexpr i32 c_LODNumCells = Chunk::Size();
  static constexpr BlockBox c_LODCellBounds(0, c_LODNumCells - 1);
  static constexpr BlockBox c_LODSampleBounds(0, c_LODNumCells);
  static constexpr f32 c_TransitionCellFractionalWidth = 0.5f;
  static constexpr i32 c_MinLODLevel = 1;

  // Rendering
  static constexpr i32 c_SSBOBinding = 2;
  static constexpr u32 c_SSBOSize = eng::math::pow2<u32>(20);
  static std::unique_ptr<eng::Shader> s_Shader;
  static std::unique_ptr<eng::ShaderBufferStorage> s_SSBO;
  static const eng::mem::BufferLayout s_VertexBufferLayout = { { eng::mem::DataType::Float3, "a_Position"     },
                                                               { eng::mem::DataType::Float3, "a_IsoNormal"    },
                                                               { eng::mem::DataType::Int,    "a_BlockIndex" } };

  struct NoiseData
  {
    eng::math::Vec3 position;
    eng::math::Vec3 normal;
    block::Type blockType;
  };

  struct SurfaceData
  {
    length_t elevation;
    block::Type blockType;
  };

  static constexpr BlockIndex transitionCellFaceIndexToSampleIndex(const BlockIndex& cellIndex, i32 faceIndex, eng::math::Direction face)
  {
    return cellIndex + BlockIndex(0, faceIndex % 3, faceIndex / 3).permute(eng::math::axisOf(face));
  }

  static BlockArrayRect<SurfaceData> generateNoise(const NodeID& node)
  {
    length_t cellLength = node.length() / c_LODNumCells;
    eng::math::Vec2 lodAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node.anchor());

    BlockArrayRect<SurfaceData> noiseValues(static_cast<BlockRect>(c_LODSampleBounds), eng::AllocationPolicy::ForOverwrite);
    noiseValues.populate([cellLength, lodAnchorXY](BlockIndex2D index)
    {
      // Sample noise at cell corners
      eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(index);
      biome::PropertyVector terrainProperties = terrain::terrainPropertiesAt(pointXY);
      biome::ID biome = terrain::biomeAt(terrainProperties);
      return SurfaceData(terrain::getApproximateElevation(pointXY), terrain::getApproximateBlockType(biome));
    });
    return noiseValues;
  }

  static bool needsMesh(const NodeID& node, const BlockArrayRect<SurfaceData>& noiseValues)
  {
    length_t lodFloor = node.anchor().k * Chunk::Length();
    length_t lodCeiling = lodFloor + node.length();

    // Check if LOD is fully below or above surface, if so, no need to generate mesh
    return eng::algo::anyOf(noiseValues, [lodFloor, lodCeiling](const SurfaceData& surfaceData)
    {
      length_t terrainHeight = surfaceData.elevation;
      return lodFloor <= terrainHeight && terrainHeight <= lodCeiling;
    });
  }

  static BlockArrayRect<eng::math::Vec3> calculateNoiseNormals(const NodeID& node, const BlockArrayRect<SurfaceData>& noiseValues)
  {
    length_t cellLength = node.length() / c_LODNumCells;
    eng::math::Vec2 lodAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node.anchor());

    // Calculate normals using central differences
    BlockArrayRect<eng::math::Vec3> noiseNormals(static_cast<BlockRect>(c_LODSampleBounds), eng::AllocationPolicy::ForOverwrite);
    noiseNormals.populate([cellLength, lodAnchorXY, &noiseValues](BlockIndex2D index)
    {
      // Surface heights in adjacent positions.  L - lower, C - center, U - upper
      length_t fLC = 0_m, fUC = 0_m, fCL = 0_m, fCU = 0_m;

      // TODO: Replace with new elevation system
      BlockIndex2D indexLC = index + BlockIndex2D(-1, 0);
      if (index.i == 0)
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexLC);
        fLC = terrain::getApproximateElevation(pointXY);
      }
      else
        fLC = noiseValues(indexLC).elevation;

      BlockIndex2D indexUC = index + BlockIndex2D(1, 0);
      if (index.i == c_LODNumCells)
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexUC);
        fUC = terrain::getApproximateElevation(pointXY);
      }
      else
        fUC = noiseValues(indexUC).elevation;

      BlockIndex2D indexCL = index + BlockIndex2D(0, -1);
      if (index.j == 0)
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexCL);
        fCL = terrain::getApproximateElevation(pointXY);
      }
      else
        fCL = noiseValues(indexCL).elevation;

      BlockIndex2D indexCU = index + BlockIndex2D(0, 1);
      if (index.j == c_LODNumCells)
      {
        eng::math::Vec2 pointXY = lodAnchorXY + cellLength * static_cast<eng::math::Vec2>(indexCU);
        fCU = terrain::getApproximateElevation(pointXY);
      }
      else
        fCU = noiseValues(indexCU).elevation;

      eng::math::Vec2 gradient{};
      gradient.x = (fUC - fLC) / (2 * cellLength);
      gradient.y = (fCU - fCL) / (2 * cellLength);

      return glm::normalize(eng::math::Vec3(-gradient, 1));
    });
    return noiseNormals;
  }

  static NoiseData interpolateNoiseData(const NodeID& node,
                                        const BlockArrayRect<SurfaceData>& noiseValues,
                                        const BlockArrayRect<eng::math::Vec3>& noiseNormals,
                                        const BlockIndex& cornerA, const BlockIndex& cornerB)
  {
    length_t lodFloor = node.anchor().k * Chunk::Length();
    length_t cellLength = node.length() / c_LODNumCells;

    // Vertex positions
    eng::math::Vec3 posA = static_cast<eng::math::Vec3>(cornerA) * cellLength;
    eng::math::Vec3 posB = static_cast<eng::math::Vec3>(cornerB) * cellLength;

    length_t zA = lodFloor + cornerA.k * cellLength;
    length_t zB = lodFloor + cornerB.k * cellLength;

    const SurfaceData& surfaceDataA = noiseValues(static_cast<BlockIndex2D>(cornerA));
    const SurfaceData& surfaceDataB = noiseValues(static_cast<BlockIndex2D>(cornerB));

    // Isovalues of corners A and B
    length_t tA = surfaceDataA.elevation - zA;
    length_t tB = surfaceDataB.elevation - zB;

    // Fraction of distance along edge vertex should be placed
    f32 t = eng::arithmeticCastUnchecked<f32>(tA / (tA - tB));
    eng::math::Vec3 vertexPosition = eng::math::lerp(posA, posB, t);

    // Estimate isosurface normal using linear interpolation between corners
    const eng::math::Vec3& n0 = noiseNormals(static_cast<BlockIndex2D>(cornerA));
    const eng::math::Vec3& n1 = noiseNormals(static_cast<BlockIndex2D>(cornerB));
    eng::math::Vec3 isoNormal = eng::math::lerp(n0, n1, t);

    block::Type blockType = t < 0.5f ? surfaceDataA.blockType : surfaceDataB.blockType;
    return { vertexPosition, glm::normalize(isoNormal), blockType };
  }

  // Formulas can be found in section 4.4 of TransVoxel paper
  static bool isVertexNearFace(bool isUpstream, f32 u, f32 cellLength)
  {
    return isUpstream ? u > cellLength * (c_LODNumCells - 1) : u < cellLength;
  }
  static f32 vertexAdjustment1D(bool isUpstream, f32 u, f32 cellLength)
  {
    return c_TransitionCellFractionalWidth * (isUpstream ? ((c_LODNumCells - 1) * cellLength - u) : (cellLength - u));
  }
  static eng::math::FMat3 calculateVertexTransform(const eng::math::Float3& n)
  {
    return eng::math::FMat3(1 - n.x * n.x, -n.x * n.y, -n.x * n.z,
                            -n.x * n.y, 1 - n.y * n.y, -n.y * n.z,
                            -n.x * n.z, -n.y * n.z, 1 - n.z * n.z);
  }

  static Vertex adjustedPrimaryVertex(const Vertex& vertex, const NodeID& node, eng::EnumBitMask<eng::math::Direction> transitionFaces)
  {
    f32 cellLength = node.length() / c_LODNumCells;

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

  static Mesh generatePrimaryMesh(const NodeID& node, const BlockArrayRect<SurfaceData>& noiseValues, const BlockArrayRect<eng::math::Vec3>& noiseNormals)
  {
    ENG_PROFILE_FUNCTION();

    struct VertexReuseData
    {
      u32 baseMeshIndex = 0;
      std::array<i8, 4> vertexOrder = { -1, -1, -1, -1 };
    };
    using VertexLayer = std::array<std::array<VertexReuseData, c_LODNumCells>, c_LODNumCells>;

    length_t cellLength = node.length() / c_LODNumCells;

    Mesh primaryMesh;
    VertexLayer prevLayer{};
    VertexLayer currLayer{};
    for (const BlockIndex& cellIndex : c_LODCellBounds)
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
        if (noiseValues(static_cast<BlockIndex2D>(sampleIndex)).elevation > sampleZ)
          cellCase |= eng::bit(cornerIndex);
      }
      if (cellCase == 0 || cellCase == 255) // These cases don't have triangles
        continue;

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
        NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, cornerIndexA, cornerIndexB);

        u32 vertexCount = eng::arithmeticCastUnchecked<u32>(primaryMesh.vertices.size());
        primaryMesh.vertices.emplace_back(noiseData.position, noiseData.normal, noiseData.blockType);
        primaryMesh.indices.push_back(vertexCount);
        prevCellVertexIndices[edgeIndex] = vertexCount;

        cellVertexCount++;
      }
    }
    return primaryMesh;
  }

  static Mesh generateTransitionMesh(const NodeID& node,
                                     const BlockArrayRect<SurfaceData>& noiseValues,
                                     const BlockArrayRect<eng::math::Vec3>& noiseNormals,
                                     eng::math::Direction face)
  {
    ENG_PROFILE_FUNCTION();

    static constexpr i32 numLowResultionCells = c_LODNumCells / 2;
    static constexpr BlockRect lowResolutionCellBounds(0, numLowResultionCells - 1);

    struct VertexReuseData
    {
      u32 baseMeshIndex = 0;
      std::array<i8, 10> vertexOrder = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    };
    using VertexStrip = std::array<VertexReuseData, numLowResultionCells>;

    length_t cellLength = node.length() / c_LODNumCells;

    // Generate transition mesh using Transvoxel algorithm
    Mesh transitionMesh;
    VertexStrip prevStrip{};
    VertexStrip currStrip{};
    for (BlockIndex2D lowResolutionCellIndex : lowResolutionCellBounds)
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
        if (noiseValues[sampleIndex.i][sampleIndex.j].elevation > sampleZ)
          cellCase |= eng::bit(c_TransitionCellFaceIndexToBitFlip[transitionCellFaceIndex]);
      }
      if (cellCase == 0 || cellCase == 511) // These cases don't have triangles
        continue;

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
        bool isOnLowResolutionSide = sampleNibbleB > 8;

        // Indices of samples A,B
        BlockIndex sampleIndexA = transitionCellFaceIndexToSampleIndex(cellIndex, c_TransitionCellSampleIndexToTransitionCellFaceIndex[sampleNibbleA], face);
        BlockIndex sampleIndexB = transitionCellFaceIndexToSampleIndex(cellIndex, c_TransitionCellSampleIndexToTransitionCellFaceIndex[sampleNibbleB], face);
        NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, sampleIndexA, sampleIndexB);

        if (!isOnLowResolutionSide)
          noiseData.position -= c_TransitionCellFractionalWidth * cellLength * static_cast<eng::math::Vec3>(BlockIndex::Dir(face));

        u32 vertexCount = eng::arithmeticCastUnchecked<u32>(transitionMesh.vertices.size());
        transitionMesh.vertices.emplace_back(noiseData.position, noiseData.normal, noiseData.blockType);
        transitionMesh.indices.push_back(vertexCount);
        prevCellVertexIndices[edgeIndex] = vertexCount;

        cellVertexCount++;
      }
    }

    // If transition mesh is on downstream side, we must reverse triangle orientation
    if (!isUpstream(face))
      eng::algo::reverse(transitionMesh.indices);

    return transitionMesh;
  }

  static std::shared_ptr<RenderData> generateRenderData(const NodeID& node)
  {
    ENG_PROFILE_FUNCTION();

    // Generate voxel data using heightmap
    BlockArrayRect<SurfaceData> noiseValues = generateNoise(node);

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

  static bool shouldBeDivided(const Node& node, const GlobalIndex& originIndex)
  {
    globalIndex_t splitRange = node.id.size() + param::RenderDistance();
    GlobalBox splitRangeBoundingBox(originIndex - splitRange, originIndex + splitRange);
    return splitRangeBoundingBox.overlapsWith(node.id.boundingBox());
  }



  LODManager::LODManager()
    : m_MultiDrawArray(s_VertexBufferLayout),
      m_ThreadPool("LOD Manager", 1),
      m_Root(c_RootNodeID)
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
      m_UpdateFuture = m_ThreadPool.submit(eng::thread::Priority::Low, &LODManager::updateTask, this, player::originIndex());

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

  void LODManager::updateTask(const GlobalIndex& originIndex)
  {
    ENG_PROFILE_FUNCTION();

    std::vector<Node*> nodes = reverseLevelOrder();
    std::vector<Node*>::iterator branchNodesBegin = eng::algo::partition(nodes, [this, &originIndex](Node* node)
    {
      if (node->isLeaf() && node->id.lodLevel() > c_MinLODLevel)
        tryDivide(*node, originIndex);
      return node->isLeaf();
    });
    std::for_each(branchNodesBegin, nodes.end(), [this, &originIndex](Node* node)
    {
      if (!node->hasGrandChildren())
        tryCombine(*node, originIndex);
    });
  }

  bool LODManager::tryDivide(Node& node, const GlobalIndex& originIndex)
  {
    if (!shouldBeDivided(node, originIndex))
      return false;

    i32 dividedLodLevel = node.id.lodLevel() - 1;
    for (const std::optional<NodeID>& neighbor : neighborQuery(node))
      if (neighbor && std::abs(dividedLodLevel - neighbor->lodLevel()) > 1)
        return false;

    ENG_PROFILE_FUNCTION();

    std::vector<DrawCommand> newDrawCommands;
    std::vector<NodeID> removedNodes = { node.id };

    node.divide();
    for (Node& child : node.children)
      if (std::optional<DrawCommand> drawCommand = createNewMesh(child))
        newDrawCommands.push_back(std::move(*drawCommand));

    for (const GlobalBox& adjustmentRegion : eng::math::FaceInteriors(node.id.boundingBox().expand()))
      createAdjustedMeshesInRegion(newDrawCommands, adjustmentRegion);

    m_StateChangeQueue.emplace(std::move(newDrawCommands), std::move(removedNodes));
    return true;
  }

  bool LODManager::tryCombine(Node& parentNode, const GlobalIndex& originIndex)
  {
    if (shouldBeDivided(parentNode, originIndex))
      return false;

    for (const std::optional<NodeID>& neighbor : neighborQuery(parentNode))
      if (neighbor && std::abs(parentNode.id.lodLevel() - neighbor->lodLevel()) > 1)
        return false;

    for (const GlobalBox& faceNeighborRegion : eng::math::FaceInteriors(parentNode.id.boundingBox().expand()))
    {
      std::vector<NodeID> faceNeighbors = find(faceNeighborRegion);
      for (const NodeID& neighbor : faceNeighbors)
        if (neighbor.lodLevel() != faceNeighbors.front().lodLevel())
          return false;
    }

    ENG_PROFILE_FUNCTION();

    std::vector<DrawCommand> newDrawCommands;
    std::vector<NodeID> removedNodes;

    for (Node& child : parentNode.children)
      removedNodes.push_back(child.id);
    parentNode.combine();

    std::optional<DrawCommand> drawCommand = createNewMesh(parentNode);
    if (drawCommand)
      newDrawCommands.push_back(std::move(*drawCommand));

    for (const GlobalBox& adjustmentRegion : eng::math::FaceInteriors(parentNode.id.boundingBox().expand()))
      createAdjustedMeshesInRegion(newDrawCommands, adjustmentRegion);

    m_StateChangeQueue.emplace(std::move(newDrawCommands), std::move(removedNodes));
    return true;
  }

  eng::EnumArray<std::optional<NodeID>, eng::math::Direction> LODManager::neighborQuery(const Node& node) const
  {
    eng::EnumArray<std::optional<NodeID>, eng::math::Direction> neighbors;
    for (eng::math::Direction face : eng::math::Directions())
    {
      GlobalIndex offset = (eng::math::isUpstream(face) ? node.id.size() : 1) * GlobalIndex::Dir(face);
      neighbors[face] = find(node.id.anchor() + offset);
    }
    return neighbors;
  }

  eng::EnumBitMask<eng::math::Direction> LODManager::transitionNeighbors(const Node& node) const
  {
    eng::EnumArray<std::optional<NodeID>, eng::math::Direction> neighborQueryResult = neighborQuery(node);

    eng::EnumBitMask<eng::math::Direction> transitionNeighbors;
    for (eng::math::Direction face : eng::math::Directions())
    {
      const std::optional<NodeID>& neighbor = neighborQueryResult[face];
      if (!neighbor)
        continue;

      if (neighbor->lodLevel() - node.id.lodLevel() == 1)
        transitionNeighbors.set(face);
      else if (neighbor->lodLevel() - node.id.lodLevel() > 1)
        ENG_WARN("LOD neighbor is more than one level lower resolution!");
    }
    return transitionNeighbors;
  }

  std::optional<NodeID> LODManager::find(const GlobalIndex& index) const
  {
    if (!c_OctreeBounds.encloses(index))
      return std::nullopt;

    auto search = [&index](const Node& node, auto search)
    {
      if (node.isLeaf())
        return node.id;

      uSize childIndex = Node::ChildBounds().linearIndexOf(node.id.childIndex(index));
      return search(node.children[childIndex], search);
    };
    return search(m_Root, search);
  }

  std::vector<NodeID> LODManager::find(const GlobalBox& region) const
  {
    std::vector<NodeID> foundNodes;
    auto search = [&region, &foundNodes](const Node& node, const auto& search)
    {
      if (!region.overlapsWith(node.id.boundingBox()))
        return;

      if (node.isLeaf())
        foundNodes.push_back(node.id);
      else
        for (const Node& child : node.children)
          search(child, search);
    };
    search(m_Root, search);
    return foundNodes;
  }

  std::optional<DrawCommand> LODManager::createNewMesh(Node& node)
  {
    if (node.id.lodLevel() > param::HighestRenderableLODLevel())
      return std::nullopt;

    node.data = generateRenderData(node.id);
    if (!node.data)
      return std::nullopt;

    eng::EnumBitMask<eng::math::Direction> transitionFaces = transitionNeighbors(node);
    return createDrawCommand(*node.data, node.id, transitionFaces);
  }

  std::optional<DrawCommand> LODManager::createAdjustedMesh(const Node& node) const
  {
    if (!node.data)
      return std::nullopt;

    eng::EnumBitMask<eng::math::Direction> transitionFaces = transitionNeighbors(node);
    return createDrawCommand(*node.data, node.id, transitionFaces);
  }

  void LODManager::createAdjustedMeshesInRegion(std::vector<DrawCommand>& drawCommands, const GlobalBox& region) const
  {
    auto implementation = [this, &drawCommands, &region](const Node& node, const auto& implementation)
    {
      if (!region.overlapsWith(node.id.boundingBox()))
        return;

      if (node.isLeaf())
      {
        if (std::optional<DrawCommand> drawCommand = createAdjustedMesh(node))
          drawCommands.push_back(std::move(*drawCommand));
      }
      else
        for (const Node& child : node.children)
          implementation(child, implementation);
    };
    implementation(m_Root, implementation);
  }

  std::vector<Node*> LODManager::reverseLevelOrder()
  {
    std::vector<Node*> nodes;
    auto gatherLeaves = [&nodes](Node& node, auto gatherLeaves) -> void
    {
      nodes.push_back(&node);
      if (!node.isLeaf())
        for (Node& child : node.children)
          gatherLeaves(child, gatherLeaves);
    };
    gatherLeaves(m_Root, gatherLeaves);

    eng::algo::sort(nodes, [](Node* node) { return node->id.lodLevel(); }, eng::SortPolicy::Ascending);
    return nodes;
  }

  void LODManager::checkState() const
  {
    auto check = [this](const Node& node, auto check)
    {
      if (node.isLeaf())
      {
        for (const GlobalBox& neighborBox : eng::math::FaceInteriors(node.id.boundingBox().expand()))
          for (const NodeID& neighbor : find(neighborBox))
            if (std::abs(node.id.lodLevel() - neighbor.lodLevel()) > 1)
              ENG_ERROR("LOD tree is in incorrect state!");
        return;
      }
      for (const Node& child : node.children)
        check(child, check);
    };
    check(m_Root, check);
  }
}