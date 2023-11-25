#include "GMpch.h"
#include "LODManager.h"
#include "GlobalParameters.h"
#include "Indexing/Operations.h"
#include "Player/Player.h"
#include "Util/TransVoxel.h"
#include "World/Terrain.h"

namespace newLod
{
  static constexpr i32 c_NumCells = Chunk::Size();
  static constexpr BlockBox c_LODBounds(0, c_NumCells);
  static constexpr BlockRect c_LODBounds2D(0, c_NumCells);

  // Width of a transition cell as a fraction of regular cell width
  static constexpr length_t c_TransitionCellFractionalWidth = 0.5_m;

  // Rendering
  static constexpr i32 c_StorageBufferBinding = 1;
  static constexpr u32 c_StorageBufferSize = 0;
  static std::unique_ptr<eng::Shader> s_Shader;
  static std::unique_ptr<eng::mem::StorageBuffer> s_SSBO;
  static const eng::mem::BufferLayout s_VertexBufferLayout = { { eng::mem::ShaderDataType::Float3, "a_Position"       },
                                                               { eng::mem::ShaderDataType::Float3, "a_IsoNormal"      },
                                                               { eng::mem::ShaderDataType::Int2,   "a_TextureIndices" },
                                                               { eng::mem::ShaderDataType::Float2, "a_TextureWeighs"  },
                                                               { eng::mem::ShaderDataType::Int,    "a_QuadIndex"      } };

  struct NoiseData
  {
    eng::math::Vec3 position;
    eng::math::Vec3 normal;

    terrain::CompoundSurfaceData surfaceData;
  };

  struct UniformData
  {
    eng::math::Float3 anchor;
    f32 textureScaling;
    const f32 nearPlaneDistance = 10 * block::lengthF();
    const f32 farPlaneDistance = 1e10f * block::lengthF();
  };

  // LOD smoothness parameter, must be in the range [0.0, 1.0]
  static constexpr f32 smoothnessLevel(i32 nodeDepth)
  {
    return std::min(0.15f * Octree::LODLevel(nodeDepth) + 0.3f, 1.0f);
  }

  // Calculate quantity based on values at corners that compose an edge.  The smoothness parameter s is used to interpolate between 
  // roughest iso-surface (vertex is always chosen at edge midpoint) and the smooth iso-surface interpolation used by Paul Bourke.
  template<typename T>
  static T semiSmoothInterpolation(const T& q0, const T& q1, f32 t, f32 smoothness)
  {
    return ((1 - smoothness) / 2 + smoothness * (1 - t)) * q0 + ((1 - smoothness) / 2 + smoothness * t) * q1;
  }

  static BlockArrayRect<terrain::CompoundSurfaceData> generateNoise(const NodeID& node)
  {
    length_t cellLength = node.length() / c_NumCells;
    eng::math::Vec2 LODAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node.anchor());

    BlockArrayRect<terrain::CompoundSurfaceData> noiseValues(c_LODBounds2D, eng::AllocationPolicy::ForOverwrite);
    for (i32 i = 0; i < c_NumCells + 1; ++i)
      for (i32 j = 0; j < c_NumCells + 1; ++j)
      {
        // Sample noise at cell corners
        eng::math::Vec2 pointXY = LODAnchorXY + cellLength * eng::math::Vec2(i, j);
        noiseValues[i][j] = terrain::getSurfaceInfo(pointXY);
      }
    return noiseValues;
  }

  static bool needsMesh(const NodeID& node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t LODFloor = node.anchor().k * Chunk::Length();
    length_t LODCeiling = LODFloor + node.length();

    // Check if LOD is fully below or above surface, if so, no need to generate mesh
    bool needsMesh = false;
    for (i32 i = 0; i < c_NumCells + 1; ++i)
      for (i32 j = 0; j < c_NumCells + 1; ++j)
      {
        length_t terrainHeight = noiseValues[i][j].getElevation();

        if (LODFloor <= terrainHeight && terrainHeight <= LODCeiling)
        {
          needsMesh = true;
          goto endCheck;
        }
      }
  endCheck:;

    return needsMesh;
  }

  static BlockArrayRect<eng::math::Vec3> calcNoiseNormals(const NodeID& node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t cellLength = node.length() / c_NumCells;
    eng::math::Vec2 LODAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node.anchor());

    // Calculate normals using central differences
    BlockArrayRect<eng::math::Vec3> noiseNormals(c_LODBounds2D, eng::AllocationPolicy::ForOverwrite);
    for (i32 i = 0; i < c_NumCells + 1; ++i)
      for (i32 j = 0; j < c_NumCells + 1; ++j)
      {
        // Surface heights in adjacent positions.  L - lower, C - center, U - upper
        length_t fLC = 0_m, fUC = 0_m, fCL = 0_m, fCU = 0_m;

        // TODO: Replace with new elevation system
        if (i == 0)
        {
          eng::math::Vec2 pointXY = LODAnchorXY + cellLength * eng::math::Vec2(i - 1, j);
          fLC = terrain::getElevation(pointXY);
        }
        else
          fLC = noiseValues[i - 1][j].getElevation();

        if (i == c_NumCells)
        {
          eng::math::Vec2 pointXY = LODAnchorXY + cellLength * eng::math::Vec2(i + 1, j);
          fUC = terrain::getElevation(pointXY);
        }
        else
          fUC = noiseValues[i + 1][j].getElevation();

        if (j == 0)
        {
          eng::math::Vec2 pointXY = LODAnchorXY + cellLength * eng::math::Vec2(i, j - 1);
          fCL = terrain::getElevation(pointXY);
        }
        else
          fCL = noiseValues[i][j - 1].getElevation();

        if (j == c_NumCells)
        {
          eng::math::Vec2 pointXY = LODAnchorXY + cellLength * eng::math::Vec2(i, j + 1);
          fCU = terrain::getElevation(pointXY);
        }
        else
          fCU = noiseValues[i][j + 1].getElevation();

        eng::math::Vec2 gradient{};
        gradient.x = (fUC - fLC) / (2 * cellLength);
        gradient.y = (fCU - fCL) / (2 * cellLength);

        eng::math::Vec3 normal = glm::normalize(eng::math::Vec3(-gradient, 1));
        noiseNormals[i][j] = normal;
      }
    return noiseNormals;
  }

  static NoiseData interpolateNoiseData(const NodeID& node,
                                        const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues,
                                        const BlockArrayRect<eng::math::Vec3>& noiseNormals,
                                        const BlockIndex& cornerA, const BlockIndex& cornerB, f32 smoothness)
  {
    length_t LODFloor = node.anchor().k * Chunk::Length();
    length_t cellLength = node.length() / c_NumCells;

    // Vertex positions
    eng::math::Vec3 posA = static_cast<eng::math::Vec3>(cornerA) * cellLength;
    eng::math::Vec3 posB = static_cast<eng::math::Vec3>(cornerB) * cellLength;

    length_t zA = LODFloor + cornerA.k * cellLength;
    length_t zB = LODFloor + cornerB.k * cellLength;

    const terrain::CompoundSurfaceData& surfaceDataA = noiseValues[cornerA.i][cornerA.j];
    const terrain::CompoundSurfaceData& surfaceDataB = noiseValues[cornerB.i][cornerB.j];

    // Isovalues of corners A and B
    length_t tA = surfaceDataA.getElevation() - zA;
    length_t tB = surfaceDataB.getElevation() - zB;

    // Fraction of distance along edge vertex should be placed
    f32 t = eng::arithmeticCastUnchecked<f32>(tA / (tA - tB));

    eng::math::Vec3 vertexPosition = semiSmoothInterpolation(posA, posB, t, smoothness);
    terrain::CompoundSurfaceData surfaceData = semiSmoothInterpolation(surfaceDataA, surfaceDataB, t, smoothness);

    // Estimate isosurface normal using linear interpolation between corners
    const eng::math::Vec3& n0 = noiseNormals[cornerA.i][cornerA.j];
    const eng::math::Vec3& n1 = noiseNormals[cornerB.i][cornerB.j];
    eng::math::Vec3 isoNormal = semiSmoothInterpolation(n0, n1, t, smoothness);

    return { vertexPosition, isoNormal, surfaceData };
  }

  // Generate primary LOD mesh using Marching Cubes algorithm
  static DrawCommand generatePrimaryMesh(const NodeID& node,
                                         const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues,
                                         const BlockArrayRect<eng::math::Vec3>& noiseNormals)
  {
    ENG_PROFILE_FUNCTION();

    struct VertexReuseData
    {
      u32 baseMeshIndex = 0;
      i8 vertexOrder[4] = { -1, -1, -1, -1 };
    };
    using VertexLayer = eng::math::ArrayRect<VertexReuseData, blockIndex_t>;

    length_t LODFloor = node.anchor().k * Chunk::Length();
    length_t cellLength = node.length() / c_NumCells;
    f32 smoothness = smoothnessLevel(node.lodLevel());

    i32 vertexCount = 0;
    std::vector<u32> primaryMeshIndices;
    std::vector<Vertex> primaryMeshVertices;
    VertexLayer prevLayer(Chunk::Bounds2D(), eng::AllocationPolicy::DefaultInitialize);
    for (i32 i = 0; i < c_NumCells; ++i)
    {
      VertexLayer currLayer(Chunk::Bounds2D(), eng::AllocationPolicy::DefaultInitialize);

      for (i32 j = 0; j < c_NumCells; ++j)
        for (i32 k = 0; k < c_NumCells; ++k)
        {
          // Determine which of the 256 cases the cell belongs to
          u8 cellCase = 0;
          for (i32 v = 0; v < 8; ++v)
          {
            // Cell corner indices and z-position
            i32 I = v & eng::bit(0) ? i + 1 : i;
            i32 J = v & eng::bit(1) ? j + 1 : j;
            i32 K = v & eng::bit(2) ? k + 1 : k;
            length_t Z = LODFloor + K * cellLength;

            if (noiseValues[I][J].getElevation() > Z)
              cellCase |= eng::bit(v);
          }
          if (cellCase == 0 || cellCase == 255)
            continue;

          currLayer[j][k].baseMeshIndex = vertexCount;

          // Use lookup table to determine which of 15 equivalence classes the cell belongs to
          u8 cellEquivClass = c_RegularCellClass[cellCase];
          RegularCellData cellData = c_RegularCellData[cellEquivClass];
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
              primaryMeshIndices.push_back(vertexIndex);
              continue;
            }

            // Lookup placement of corners A,B that form the cell edge new vertex lies on
            u16 vertexData = c_RegularVertexData[cellCase][edgeIndex];
            u8 sharedVertexIndex = (vertexData & 0x0F00) >> 8;
            u8 sharedVertexDirection = (vertexData & 0xF000) >> 12;
            bool newVertex = sharedVertexDirection == 8;

            // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
            if (newVertex)
              currLayer[j][k].vertexOrder[sharedVertexIndex] = cellVertexCount;
            else
            {
              i32 I = sharedVertexDirection & eng::bit(0) ? i - 1 : i;
              i32 J = sharedVertexDirection & eng::bit(1) ? j - 1 : j;
              i32 K = sharedVertexDirection & eng::bit(2) ? k - 1 : k;

              if (I >= 0 && J >= 0 && K >= 0)
              {
                const auto& targetLayer = I == i ? currLayer : prevLayer;

                i32 baseMeshIndex = targetLayer[J][K].baseMeshIndex;
                i32 vertexOrder = targetLayer[J][K].vertexOrder[sharedVertexIndex];
                if (baseMeshIndex > 0 && vertexOrder >= 0)
                {
                  primaryMeshIndices.push_back(baseMeshIndex + vertexOrder);
                  prevCellVertexIndices[edgeIndex] = baseMeshIndex + vertexOrder;
                  continue;
                }
              }
            }

            u8 cornerIndexA = vertexData & 0x000F;
            u8 cornerIndexB = (vertexData & 0x00F0) >> 4;

            // Indices of corners A,B
            BlockIndex cornerA{};
            cornerA.i = cornerIndexA & eng::bit(0) ? i + 1 : i;
            cornerA.j = cornerIndexA & eng::bit(1) ? j + 1 : j;
            cornerA.k = cornerIndexA & eng::bit(2) ? k + 1 : k;
            BlockIndex cornerB{};
            cornerB.i = cornerIndexB & eng::bit(0) ? i + 1 : i;
            cornerB.j = cornerIndexB & eng::bit(1) ? j + 1 : j;
            cornerB.k = cornerIndexB & eng::bit(2) ? k + 1 : k;

            NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, cornerA, cornerB, smoothness);

            primaryMeshVertices.emplace_back(noiseData.position, noiseData.normal, noiseData.surfaceData.getTextureIndices(), noiseData.surfaceData.getTextureWeights(), vert % 3);
            primaryMeshIndices.push_back(vertexCount);
            prevCellVertexIndices[edgeIndex] = vertexCount;

            vertexCount++;
            cellVertexCount++;
          }
        }

      prevLayer = std::move(currLayer);
    }

    MeshID meshID(node);
    return DrawCommand(meshID, std::move(primaryMeshIndices), std::move(primaryMeshVertices));
  }

  // Generate transition meshes using Transvoxel algorithm
  static DrawCommand generateTransitionMesh(const NodeID& node,
                                            const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues,
                                            const BlockArrayRect<eng::math::Vec3>& noiseNormals, eng::math::Direction face)
  {
    ENG_PROFILE_FUNCTION();

    struct VertexReuseData
    {
      u32 baseMeshIndex = 0;
      i8 vertexOrder[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    };

    static constexpr eng::EnumArray<eng::math::Vec3, eng::math::Direction> normals = 
      { { eng::math::Direction::West,   {-1,  0,  0} },
        { eng::math::Direction::East,   { 1,  0,  0} },
        { eng::math::Direction::South,  { 0, -1,  0} },
        { eng::math::Direction::North,  { 0,  1,  0} },
        { eng::math::Direction::Bottom, { 0,  0, -1} },
        { eng::math::Direction::Top,    { 0,  0,  1} } };

    length_t LODFloor = node.anchor().k * Chunk::Length();
    length_t cellLength = node.length() / c_NumCells;
    length_t transitionCellWidth = c_TransitionCellFractionalWidth * cellLength;

    // Relabel coordinates, u being the coordinate normal to face
    eng::math::Axis u = axisOf(face);
    eng::math::Axis v = cycle(u);
    eng::math::Axis w = cycle(v);
    i32 uIndex = isUpstream(face) ? c_NumCells : 0;

    // Generate transition mesh using Transvoxel algorithm
    i32 vertexCount = 0;
    std::vector<u32> transitionMeshIndices;
    std::vector<Vertex> transitionMeshVertices;
    std::array<VertexReuseData, c_NumCells / 2> prevRow{};
    for (i32 i = 0; i < c_NumCells; i += 2)
    {
      std::array<VertexReuseData, c_NumCells / 2> currRow{};

      for (i32 j = 0; j < c_NumCells; j += 2)
      {
        // Determine which of the 512 cases the cell belongs to
        u16 cellCase = 0;
        for (i32 p = 0; p < 9; ++p)
        {
          BlockIndex sample{};
          sample[u] = uIndex;
          sample[v] = i + p % 3;
          sample[w] = j + p / 3;

          // If block face normal along negative axis, we must reverse v coordinate-axis to ensure correct orientation
          if (!isUpstream(face))
            sample[v] = c_NumCells - sample[v];

          const i32& I = sample.i;
          const i32& J = sample.j;
          const i32& K = sample.k;
          length_t Z = LODFloor + K * cellLength;

          if (noiseValues[I][J].getElevation() > Z)
            cellCase |= eng::bit(c_SampleIndexToBitFlip[p]);
        }
        if (cellCase == 0 || cellCase == 511)
          continue;

        currRow[j / 2].baseMeshIndex = vertexCount;

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
            transitionMeshIndices.push_back(vertexIndex);
            continue;
          }

          // Lookup indices of vertices A,B of the cell edge that vertex v lies on
          u16 vertexData = c_TransitionVertexData[cellCase][edgeIndex];
          u8 sharedVertexIndex = (vertexData & 0x0F00) >> 8;
          u8 sharedVertexDirection = (vertexData & 0xF000) >> 12;
          bool isReusable = sharedVertexDirection != 4;
          bool newVertex = sharedVertexDirection == 8;

          // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
          if (newVertex)
            currRow[j / 2].vertexOrder[sharedVertexIndex] = cellVertexCount;
          else if (isReusable)
          {
            i32 I = sharedVertexDirection & eng::bit(0) ? i / 2 - 1 : i / 2;
            i32 J = sharedVertexDirection & eng::bit(1) ? j / 2 - 1 : j / 2;

            if (I >= 0 && J >= 0)
            {
              const auto& targetRow = I == i / 2 ? currRow : prevRow;

              i32 baseMeshIndex = targetRow[J].baseMeshIndex;
              i32 vertexOrder = targetRow[J].vertexOrder[sharedVertexIndex];
              if (baseMeshIndex > 0 && vertexOrder >= 0)
              {
                transitionMeshIndices.push_back(baseMeshIndex + vertexOrder);
                prevCellVertexIndices[edgeIndex] = baseMeshIndex + vertexOrder;
                continue;
              }
            }
          }

          u8 cornerIndexA = vertexData & 0x000F;
          u8 cornerIndexB = (vertexData & 0x00F0) >> 4;
          bool isOnLowResSide = cornerIndexB > 8;

          // Indices of samples A,B
          BlockIndex sampleA{};
          sampleA[u] = uIndex;
          sampleA[v] = i + c_CornerIndexToSampleIndex[cornerIndexA] % 3;
          sampleA[w] = j + c_CornerIndexToSampleIndex[cornerIndexA] / 3;
          BlockIndex sampleB{};
          sampleB[u] = uIndex;
          sampleB[v] = i + c_CornerIndexToSampleIndex[cornerIndexB] % 3;
          sampleB[w] = j + c_CornerIndexToSampleIndex[cornerIndexB] / 3;

          // If block face normal along negative axis, we must reverse v coordinate-axis to ensure correct orientation
          if (!isUpstream(face))
          {
            sampleA[v] = c_NumCells - sampleA[v];
            sampleB[v] = c_NumCells - sampleB[v];
          }

          // If vertex is on low-resolution side, use smoothness level of low-resolution LOD
          f32 smoothness = smoothnessLevel(node.lodLevel() + isOnLowResSide);

          NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, sampleA, sampleB, smoothness);
          if (!isOnLowResSide)
            noiseData.position -= transitionCellWidth * normals[face];

          transitionMeshVertices.emplace_back(noiseData.position, noiseData.normal, noiseData.surfaceData.getTextureIndices(), noiseData.surfaceData.getTextureWeights(), vert % 3);
          transitionMeshIndices.push_back(vertexCount);
          prevCellVertexIndices[edgeIndex] = vertexCount;

          vertexCount++;
          cellVertexCount++;
        }
      }

      prevRow = std::move(currRow);
    }

    MeshID meshID(node, face);
    return DrawCommand(meshID, std::move(transitionMeshIndices), std::move(transitionMeshVertices));
  }



  LODManager::LODManager()
    : m_MultiDrawArray(std::make_shared<eng::thread::AsyncMultiDrawArray<DrawCommand>>(s_VertexBufferLayout))
  {
    ENG_PROFILE_FUNCTION();
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

    // eng::render::command::setFaceCulling(true);
    eng::render::command::setDepthWriting(true);
    eng::render::command::setUseDepthOffset(false);
    m_MultiDrawArray->drawOperation([&frustumPlanes, &shiftedFrustumPlanes, &planeNormalMagnitudes, &originIndex](eng::MultiDrawArray<DrawCommand>& multiDrawArray)
    {
      i32 commandCount = multiDrawArray.partition([&frustumPlanes, &shiftedFrustumPlanes, &planeNormalMagnitudes, &originIndex](const MeshID& meshID)
      {
        if (isInRange(meshID.nodeID().anchor(), originIndex, param::RenderDistance() - 1))
          return false;

        // Shift each plane by distance equal to radius of sphere that circumscribes LOD
        length_t LODSphereRadius = meshID.nodeID().boundingSphereRadius();
        for (eng::math::FrustumPlane plane : eng::math::FrustumPlanes())
          shiftedFrustumPlanes[plane].w = frustumPlanes[plane].w + LODSphereRadius * planeNormalMagnitudes[plane];

        return eng::math::isInFrustum(meshID.nodeID().center(originIndex), shiftedFrustumPlanes);
      });

      std::vector<eng::math::Float4> storageBufferData;
      storageBufferData.reserve(commandCount);
      const std::vector<DrawCommand>& drawCommands = multiDrawArray.getDrawCommandBuffer();
      for (i32 i = 0; i < commandCount; ++i)
      {
        eng::math::Vec3 nodeAnchorPosition = drawCommands[i].id().nodeID().anchorPosition(originIndex);
        storageBufferData.emplace_back(nodeAnchorPosition, 0);
      }

      multiDrawArray.bind();
      s_SSBO->modify(0, storageBufferData);
      eng::render::command::multiDrawIndexed(drawCommands, commandCount);
    });
  }

  void LODManager::meshLOD(const NodeID& node)
  {
    ENG_PROFILE_FUNCTION();

    // Generate voxel data using heightmap
    BlockArrayRect<terrain::CompoundSurfaceData> noiseValues = generateNoise(node);

    if (!needsMesh(node, noiseValues))
      return;

    // Generate normal data from heightmap
    BlockArrayRect<eng::math::Vec3> noiseNormals = calcNoiseNormals(node, noiseValues);

    m_MultiDrawArray->queueCommand(generatePrimaryMesh(node, noiseValues, noiseNormals));
    for (eng::math::Direction face : eng::math::Directions())
      m_MultiDrawArray->queueCommand(generateTransitionMesh(node, noiseValues, noiseNormals, face));
  }
}
