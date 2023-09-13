#include "GMpch.h"
#include "LOD.h"
#include "Terrain.h"
#include "Player/Player.h"
#include "Util/TransVoxel.h"

namespace LOD
{
  // Number of cells in each direction
  static constexpr int c_NumCells = Chunk::Size();

  // Width of a transition cell as a fraction of regular cell width
  static constexpr length_t c_TCFractionalWidth = 0.5f;

  static constexpr BlockBox c_LODBounds(0, c_NumCells);
  static constexpr BlockRect c_LODBounds2D(0, c_NumCells);

  struct NoiseData
  {
    Vec3 position;
    Vec3 normal;

    Terrain::CompoundSurfaceData surfaceData;
  };

  Vertex::Vertex(const Float3& position, const Float3& isoNormal, const std::array<int, 2>& textureIndices, const Float2& textureWeights, int quadIndex)
    : position(position), isoNormal(isoNormal), textureIndices(textureIndices), textureWeights(textureWeights), quadIndex(quadIndex) {}

  MeshData::MeshData()
    : vertices(), indices()
  {
    vertexArray = Engine::VertexArray::Create();
    vertexArray->setLayout(s_VertexBufferLayout);
  }

  Octree::Node::Node(Octree::Node* parentNode, int nodeDepth, const GlobalIndex& anchorIndex)
    : parent(parentNode), depth(nodeDepth), anchor(anchorIndex) {}

  Octree::Node::~Node()
  {
    delete data;
    data = nullptr;
    for (int i = 0; i < 8; ++i)
    {
      delete children[i];
      children[i] = nullptr;
    }
  }

  bool Octree::Node::isRoot() const { return parent == nullptr; }
  bool Octree::Node::isLeaf() const { return data != nullptr; }
  int Octree::Node::LODLevel() const { return c_MaxNodeDepth - depth; }

  globalIndex_t Octree::Node::size() const
  {
    return static_cast<globalIndex_t>(Engine::Pow2(LODLevel()));
  }

  length_t Octree::Node::length() const
  {
    return size() * Chunk::Length();
  }

  Vec3 Octree::Node::anchorPosition() const
  {
    GlobalIndex relativeIndex = anchor - Player::OriginIndex();
    return Chunk::Length() * static_cast<Vec3>(relativeIndex);
  }

  Vec3 Octree::Node::center() const
  {
    return anchorPosition() + length() / 2;
  }

  GlobalBox Octree::Node::boundingBox() const
  {
    return { anchor, anchor + size() };
  }




  Octree::Octree()
    : m_Root(Node(nullptr, 0, c_RootNodeAnchor))
  {
    m_Root.data = new Data();
  }

  void Octree::splitNode(Node* node)
  {
    EN_ASSERT(node != nullptr, "Node can't be nullptr!");
    EN_ASSERT(node->isLeaf(), "Node must be a leaf node!");
    EN_ASSERT(node->depth != c_MaxNodeDepth, "Node is already at max depth!");

    const globalIndex_t nodeChildSize = node->size() / 2;

    // Divide node into 8 equal-sized child nodes
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        for (int k = 0; k < 2; ++k)
        {
          int childIndex = i * Engine::BitUi32(2) + j * Engine::BitUi32(1) + k * Engine::BitUi32(0);
          EN_ASSERT(node->children[childIndex] == nullptr, "Child node already exists!");

          GlobalIndex nodeChildAnchor = node->anchor + nodeChildSize * GlobalIndex(i, j, k);
          node->children[childIndex] = new Node(node, node->depth + 1, nodeChildAnchor);
          node->children[childIndex]->data = new Data();
        }

    // Delete node data as it is no longer a leaf node
    delete node->data;
    node->data = nullptr;
  }

  void Octree::combineChildren(Node* node)
  {
    EN_ASSERT(node != nullptr, "Node can't be nullptr!");

    // Return if node is already a leaf
    if (node->isLeaf())
      return;

    // Delete child nodes
    for (int i = 0; i < 8; ++i)
    {
      delete node->children[i];
      node->children[i] = nullptr;
    }

    // Node becomes new leaf node
    node->data = new Data();
  }

  std::vector<Octree::Node*> Octree::getLeaves()
  {
    // Recursively collect leaf nodes
    std::vector<Node*> leaves{};
    getLeavesPriv(&m_Root, leaves);

    return leaves;
  }

  Octree::Node* Octree::findLeaf(const GlobalIndex& index)
  {
    if (index.i < m_Root.anchor.i || index.i >= m_Root.anchor.i + m_Root.size() ||
      index.j < m_Root.anchor.j || index.j >= m_Root.anchor.j + m_Root.size() ||
      index.k < m_Root.anchor.k || index.k >= m_Root.anchor.k + m_Root.size())
    {
      return nullptr;
    }

    return findLeafPriv(&m_Root, index);
  }



  void Octree::getLeavesPriv(Node* branch, std::vector<Node*>& leaves)
  {
    if (branch->isLeaf())
      leaves.push_back(branch);
    else if (branch->depth < c_MaxNodeDepth)
      for (int i = 0; i < 8; ++i)
        if (branch->children[i] != nullptr)
          getLeavesPriv(branch->children[i], leaves);
  }

  Octree::Node* Octree::findLeafPriv(Node* branch, const GlobalIndex& index)
  {
    if (branch->isLeaf())
      return branch;
    else
    {
      int i = index.i >= branch->anchor.i + branch->size() / 2;
      int j = index.j >= branch->anchor.j + branch->size() / 2;
      int k = index.k >= branch->anchor.k + branch->size() / 2;
      int childIndex = i * Engine::BitUi32(2) + j * Engine::BitUi32(1) + k * Engine::BitUi32(0);

      return findLeafPriv(branch->children[childIndex], index);
    }
  }



  void MeshData::Initialize()
  {
    s_Uniform = Engine::Uniform::Create(c_UniformBinding, sizeof(UniformData));
    s_Shader = Engine::Shader::Create("assets/shaders/ChunkLOD.glsl");
    s_TextureArray = Block::GetTextureArray();
  }

  void MeshData::BindBuffers()
  {
    s_Uniform->bind();
    s_Shader->bind();
    s_TextureArray->bind(c_TextureSlot);
  }

  void MeshData::SetUniforms(const UniformData& uniformData)
  {
    s_Uniform->set(&uniformData, sizeof(UniformData));
  }

  void Draw(const Octree::Node* leaf)
  {
    uint32_t primaryMeshIndexCount = static_cast<uint32_t>(leaf->data->primaryMesh.indices.size());

    if (primaryMeshIndexCount == 0)
      return; // Nothing to draw

    // Set local anchor position and texture scaling
    UniformData uniformData{};
    uniformData.anchor = Chunk::Length() * static_cast<Vec3>(leaf->anchor - Player::OriginIndex());
    uniformData.textureScaling = static_cast<float>(Engine::Bit(leaf->LODLevel()));
    MeshData::SetUniforms(uniformData);

    Engine::RenderCommand::DrawIndexed(leaf->data->primaryMesh.vertexArray.get(), primaryMeshIndexCount);
    for (Direction face : Directions())
    {
      int faceID = static_cast<int>(face);

      uint32_t transitionMeshIndexCount = static_cast<uint32_t>(leaf->data->transitionMeshes[faceID].indices.size());

      if (transitionMeshIndexCount == 0 || !(leaf->data->transitionFaces & Engine::Bit(faceID)))
        continue;

      Engine::RenderCommand::DrawIndexed(leaf->data->transitionMeshes[faceID].vertexArray.get(), transitionMeshIndexCount);
    }
  }



  // LOD smoothness parameter, must be in the range [0.0, 1.0]
  static constexpr float smoothnessLevel(int LODLevel)
  {
#if 1
    return std::min(0.15f * (LODLevel)+0.3f, 1.0f);
#else
    return 1.0f;
#endif
  }

  // Calculate quantity based on values at corners that compose an edge.  The smoothness parameter s is used to interpolate between 
  // roughest iso-surface (vertex is always chosen at edge midpoint) and the smooth iso-surface interpolation used by Paul Bourke.
  template<typename T>
  static T LODInterpolation(float t, float s, const T& q0, const T& q1)
  {
    return ((1 - s) / 2 + s * (1 - t)) * q0 + ((1 - s) / 2 + s * t) * q1;
  }

  static BlockArrayRect<Terrain::CompoundSurfaceData> generateNoise(Octree::Node* node)
  {
    EN_PROFILE_FUNCTION();

    length_t cellLength = node->length() / c_NumCells;
    Vec2 LODAnchorXY = Chunk::Length() * static_cast<Vec2>(node->anchor);

    BlockArrayRect<Terrain::CompoundSurfaceData> noiseValues(c_LODBounds2D, AllocationPolicy::ForOverwrite);
    for (int i = 0; i < c_NumCells + 1; ++i)
      for (int j = 0; j < c_NumCells + 1; ++j)
      {
        // Sample noise at cell corners
        Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j);

        // TODO: Replace with new terrain system
        // Noise::OctaveNoiseData<Biome::NumOctaves()> elevationData = Terrain::GetElevationData(pointXY, s_DefaultBiome);
        // float seaLevelTemperature = Terrain::GetTemperatureData(pointXY, s_DefaultBiome);
        // noiseValues[i][j] = Terrain::GetSurfaceInfo(elevationData, seaLevelTemperature, s_DefaultBiome);
      }
    return noiseValues;
  }

  static bool needsMesh(Octree::Node* node, const BlockArrayRect<Terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t LODCeiling = LODFloor + node->length();

    // Check if LOD is fully below or above surface, if so, no need to generate mesh
    bool needsMesh = false;
    for (int i = 0; i < c_NumCells + 1; ++i)
      for (int j = 0; j < c_NumCells + 1; ++j)
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

  static BlockArrayRect<Vec3> calcNoiseNormals(Octree::Node* node, const BlockArrayRect<Terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t cellLength = node->length() / c_NumCells;
    Vec2 LODAnchorXY = Chunk::Length() * static_cast<Vec2>(node->anchor);

    // Calculate normals using central differences
    BlockArrayRect<Vec3> noiseNormals(c_LODBounds2D, AllocationPolicy::ForOverwrite);
    for (int i = 0; i < c_NumCells + 1; ++i)
      for (int j = 0; j < c_NumCells + 1; ++j)
      {
        // Surface heights in adjacent positions.  L - lower, C - center, U - upper
        length_t fLC = 0_m, fUC = 0_m, fCL = 0_m, fCU = 0_m;

        // TODO: Replace with new elevation system
        if (i == 0)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i - 1, j);
          // fLC = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
        }
        else
          fLC = noiseValues[i - 1][j].getElevation();

        if (i == c_NumCells)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i + 1, j);
          // fUC = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
        }
        else
          fUC = noiseValues[i + 1][j].getElevation();

        if (j == 0)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j - 1);
          // fCL = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
        }
        else
          fCL = noiseValues[i][j - 1].getElevation();

        if (j == c_NumCells)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j + 1);
          // fCU = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
        }
        else
          fCU = noiseValues[i][j + 1].getElevation();

        Vec2 gradient{};
        gradient.x = (fUC - fLC) / (2 * cellLength);
        gradient.y = (fCU - fCL) / (2 * cellLength);

        Vec3 normal = glm::normalize(Vec3(-gradient, 1));
        noiseNormals[i][j] = normal;
      }
    return noiseNormals;
  }

  static NoiseData interpolateNoiseData(Octree::Node* node, const BlockArrayRect<Terrain::CompoundSurfaceData>& noiseValues, const BlockArrayRect<Vec3>& noiseNormals, const BlockIndex& cornerA, const BlockIndex& cornerB, float s)
  {
    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t cellLength = node->length() / c_NumCells;

    // Vertex positions
    Vec3 posA = static_cast<Vec3>(cornerA) * cellLength;
    Vec3 posB = static_cast<Vec3>(cornerB) * cellLength;

    length_t zA = LODFloor + cornerA.k * cellLength;
    length_t zB = LODFloor + cornerB.k * cellLength;

    const Terrain::CompoundSurfaceData& surfaceDataA = noiseValues[cornerA.i][cornerA.j];
    const Terrain::CompoundSurfaceData& surfaceDataB = noiseValues[cornerB.i][cornerB.j];

    // Isovalues of corners A and B
    length_t tA = surfaceDataA.getElevation() - zA;
    length_t tB = surfaceDataB.getElevation() - zB;

    // Fraction of distance along edge vertex should be placed
    float t = static_cast<float>(tA / (tA - tB));

    Vec3 vertexPosition = LODInterpolation(t, s, posA, posB);
    Terrain::CompoundSurfaceData surfaceData = LODInterpolation(t, s, surfaceDataA, surfaceDataB);

    // Estimate isosurface normal using linear interpolation between corners
    const Vec3& n0 = noiseNormals[cornerA.i][cornerA.j];
    const Vec3& n1 = noiseNormals[cornerB.i][cornerB.j];
    Vec3 isoNormal = LODInterpolation(t, s, n0, n1);

    return { vertexPosition, isoNormal, surfaceData };
  }

  // Generate primary LOD mesh using Marching Cubes algorithm
  static void generatePrimaryMesh(Octree::Node* node, const BlockArrayRect<Terrain::CompoundSurfaceData>& noiseValues, const BlockArrayRect<Vec3>& noiseNormals)
  {
    EN_PROFILE_FUNCTION();

    struct VertexReuseData
    {
      uint32_t baseMeshIndex = 0;
      int8_t vertexOrder[4] = { -1, -1, -1, -1 };
    };
    using VertexLayer = ArrayRect<VertexReuseData, blockIndex_t>;

    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t cellLength = node->length() / c_NumCells;
    float smoothness = smoothnessLevel(node->LODLevel());

    int vertexCount = 0;
    std::vector<uint32_t> primaryMeshIndices{};
    std::vector<Vertex> primaryMeshVertices{};
    VertexLayer prevLayer(Chunk::Bounds2D(), AllocationPolicy::DefaultInitialize);
    for (int i = 0; i < c_NumCells; ++i)
    {
      VertexLayer currLayer(Chunk::Bounds2D(), AllocationPolicy::DefaultInitialize);

      for (int j = 0; j < c_NumCells; ++j)
        for (int k = 0; k < c_NumCells; ++k)
        {
          // Determine which of the 256 cases the cell belongs to
          uint8_t cellCase = 0;
          for (int v = 0; v < 8; ++v)
          {
            // Cell corner indices and z-position
            int I = v & Engine::Bit(0) ? i + 1 : i;
            int J = v & Engine::Bit(1) ? j + 1 : j;
            int K = v & Engine::Bit(2) ? k + 1 : k;
            length_t Z = LODFloor + K * cellLength;

            if (noiseValues[I][J].getElevation() > Z)
              cellCase |= Engine::Bit(v);
          }
          if (cellCase == 0 || cellCase == 255)
            continue;

          currLayer[j][k].baseMeshIndex = vertexCount;

          // Use lookup table to determine which of 15 equivalence classes the cell belongs to
          uint8_t cellEquivClass = c_RegularCellClass[cellCase];
          RegularCellData cellData = c_RegularCellData[cellEquivClass];
          int triangleCount = cellData.getTriangleCount();

          // Loop over all triangles in cell
          int cellVertexCount = 0;
          std::array<uint32_t, c_MaxCellVertexCount> prevCellVertexIndices{};
          for (int vert = 0; vert < 3 * triangleCount; ++vert)
          {
            int edgeIndex = cellData.vertexIndex[vert];

            // Check if vertex has already been created in this cell
            int vertexIndex = prevCellVertexIndices[edgeIndex];
            if (vertexIndex > 0)
            {
              primaryMeshIndices.push_back(vertexIndex);
              continue;
            }

            // Lookup placement of corners A,B that form the cell edge new vertex lies on
            uint16_t vertexData = c_RegularVertexData[cellCase][edgeIndex];
            uint8_t sharedVertexIndex = (vertexData & 0x0F00) >> 8;
            uint8_t sharedVertexDirection = (vertexData & 0xF000) >> 12;
            bool newVertex = sharedVertexDirection == 8;

            // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
            if (newVertex)
              currLayer[j][k].vertexOrder[sharedVertexIndex] = cellVertexCount;
            else
            {
              int I = sharedVertexDirection & Engine::Bit(0) ? i - 1 : i;
              int J = sharedVertexDirection & Engine::Bit(1) ? j - 1 : j;
              int K = sharedVertexDirection & Engine::Bit(2) ? k - 1 : k;

              if (I >= 0 && J >= 0 && K >= 0)
              {
                const auto& targetLayer = I == i ? currLayer : prevLayer;

                int baseMeshIndex = targetLayer[J][K].baseMeshIndex;
                int vertexOrder = targetLayer[J][K].vertexOrder[sharedVertexIndex];
                if (baseMeshIndex > 0 && vertexOrder >= 0)
                {
                  primaryMeshIndices.push_back(baseMeshIndex + vertexOrder);
                  prevCellVertexIndices[edgeIndex] = baseMeshIndex + vertexOrder;
                  continue;
                }
              }
            }

            uint8_t cornerIndexA = vertexData & 0x000F;
            uint8_t cornerIndexB = (vertexData & 0x00F0) >> 4;

            // Indices of corners A,B
            BlockIndex cornerA{};
            cornerA.i = cornerIndexA & Engine::Bit(0) ? i + 1 : i;
            cornerA.j = cornerIndexA & Engine::Bit(1) ? j + 1 : j;
            cornerA.k = cornerIndexA & Engine::Bit(2) ? k + 1 : k;
            BlockIndex cornerB{};
            cornerB.i = cornerIndexB & Engine::Bit(0) ? i + 1 : i;
            cornerB.j = cornerIndexB & Engine::Bit(1) ? j + 1 : j;
            cornerB.k = cornerIndexB & Engine::Bit(2) ? k + 1 : k;

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
    node->data->primaryMesh.vertices = std::move(primaryMeshVertices);
    node->data->primaryMesh.indices = std::move(primaryMeshIndices);
  }

  // Generate transition meshes using Transvoxel algorithm
  static void generateTransitionMeshes(Octree::Node* node, const BlockArrayRect<Terrain::CompoundSurfaceData>& noiseValues, const BlockArrayRect<Vec3>& noiseNormals)
  {
    EN_PROFILE_FUNCTION();

    struct VertexReuseData
    {
      uint32_t baseMeshIndex = 0;
      int8_t vertexOrder[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    };

    static constexpr Vec3 normals[6] = { { -1, 0, 0}, { 1, 0, 0}, { 0, -1, 0}, { 0, 1, 0}, { 0, 0, -1}, { 0, 0, 1} };
                                    //       West        East        South        North       Bottom        Top

    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t cellLength = node->length() / c_NumCells;
    length_t transitionCellWidth = c_TCFractionalWidth * cellLength;

    for (Direction face : Directions())
    {
      int faceID = static_cast<int>(face);

      // Relabel coordinates, u being the coordinate normal to face
      Axis u = AxisOf(face);
      Axis v = Cycle(u);
      Axis w = Cycle(v);
      int uIndex = IsUpstream(face) ? c_NumCells : 0;

      // Generate transition mesh using Transvoxel algorithm
      int vertexCount = 0;
      std::vector<uint32_t> transitionMeshIndices{};
      std::vector<Vertex> transitionMeshVertices{};
      std::array<VertexReuseData, c_NumCells / 2> prevRow{};
      for (int i = 0; i < c_NumCells; i += 2)
      {
        std::array<VertexReuseData, c_NumCells / 2> currRow{};

        for (int j = 0; j < c_NumCells; j += 2)
        {
          // Determine which of the 512 cases the cell belongs to
          uint16_t cellCase = 0;
          for (int p = 0; p < 9; ++p)
          {
            BlockIndex sample{};
            sample[u] = uIndex;
            sample[v] = i + p % 3;
            sample[w] = j + p / 3;

            // If block face normal along negative axis, we must reverse v coordinate-axis to ensure correct orientation
            if (!IsUpstream(face))
              sample[v] = c_NumCells - sample[v];

            const int& I = sample.i;
            const int& J = sample.j;
            const int& K = sample.k;
            length_t Z = LODFloor + K * cellLength;

            if (noiseValues[I][J].getElevation() > Z)
              cellCase |= Engine::Bit(c_SampleIndexToBitFlip[p]);
          }
          if (cellCase == 0 || cellCase == 511)
            continue;

          currRow[j / 2].baseMeshIndex = vertexCount;

          // Use lookup table to determine which of 56 equivalence classes the cell belongs to
          uint8_t cellEquivClass = c_TransitionCellClass[cellCase];
          bool reverseWindingOrder = cellEquivClass >> 7;
          TransitionCellData cellData = c_TransitionCellData[cellEquivClass & 0x7F];
          int triangleCount = cellData.getTriangleCount();

          // Loop over all triangles in cell
          int cellVertexCount = 0;
          std::array<uint32_t, c_MaxCellVertexCount> prevCellVertexIndices{};
          for (int vert = 0; vert < 3 * triangleCount; ++vert)
          {
            int edgeIndex = cellData.vertexIndex[reverseWindingOrder ? 3 * triangleCount - 1 - vert : vert];

            // Check if vertex has already been created in this cell
            int vertexIndex = prevCellVertexIndices[edgeIndex];
            if (vertexIndex > 0)
            {
              transitionMeshIndices.push_back(vertexIndex);
              continue;
            }

            // Lookup indices of vertices A,B of the cell edge that vertex v lies on
            uint16_t vertexData = c_TransitionVertexData[cellCase][edgeIndex];
            uint8_t sharedVertexIndex = (vertexData & 0x0F00) >> 8;
            uint8_t sharedVertexDirection = (vertexData & 0xF000) >> 12;
            bool isReusable = sharedVertexDirection != 4;
            bool newVertex = sharedVertexDirection == 8;

            // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
            if (newVertex)
              currRow[j / 2].vertexOrder[sharedVertexIndex] = cellVertexCount;
            else if (isReusable)
            {
              int I = sharedVertexDirection & Engine::Bit(0) ? i / 2 - 1 : i / 2;
              int J = sharedVertexDirection & Engine::Bit(1) ? j / 2 - 1 : j / 2;

              if (I >= 0 && J >= 0)
              {
                const auto& targetRow = I == i / 2 ? currRow : prevRow;

                int baseMeshIndex = targetRow[J].baseMeshIndex;
                int vertexOrder = targetRow[J].vertexOrder[sharedVertexIndex];
                if (baseMeshIndex > 0 && vertexOrder >= 0)
                {
                  transitionMeshIndices.push_back(baseMeshIndex + vertexOrder);
                  prevCellVertexIndices[edgeIndex] = baseMeshIndex + vertexOrder;
                  continue;
                }
              }
            }

            uint8_t cornerIndexA = vertexData & 0x000F;
            uint8_t cornerIndexB = (vertexData & 0x00F0) >> 4;
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
            if (!IsUpstream(face))
            {
              sampleA[v] = c_NumCells - sampleA[v];
              sampleB[v] = c_NumCells - sampleB[v];
            }

            // If vertex is on low-resolution side, use smoothness level of low-resolution LOD
            float smoothness = isOnLowResSide ? smoothnessLevel(node->LODLevel() + 1) : smoothnessLevel(node->LODLevel());

            NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, sampleA, sampleB, smoothness);
            if (!isOnLowResSide)
              noiseData.position -= transitionCellWidth * normals[faceID];

            transitionMeshVertices.emplace_back(noiseData.position, noiseData.normal, noiseData.surfaceData.getTextureIndices(), noiseData.surfaceData.getTextureWeights(), vert % 3);
            transitionMeshIndices.push_back(vertexCount);
            prevCellVertexIndices[edgeIndex] = vertexCount;

            vertexCount++;
            cellVertexCount++;
          }
        }

        prevRow = std::move(currRow);
      }

      node->data->transitionMeshes[faceID].vertices = std::move(transitionMeshVertices);
      node->data->transitionMeshes[faceID].indices = std::move(transitionMeshIndices);
    }
  }

  void GenerateMesh(Octree::Node* node)
  {
    // NOTE: These values should come from biome system when implemented
    static const length_t globalMinTerrainHeight = -400 * Block::Length();
    static const length_t globalMaxTerrainHeight = 400 * Block::Length();

    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t LODCeiling = LODFloor + node->length();

    node->data->meshGenerated = true;

    // If LOD is fully below or above global min/max values, no need to generate mesh
    if (LODFloor > globalMaxTerrainHeight || LODCeiling < globalMinTerrainHeight)
      return;

    EN_PROFILE_FUNCTION();

    // Generate voxel data using heightmap
    BlockArrayRect<Terrain::CompoundSurfaceData> noiseValues = generateNoise(node);

    if (!needsMesh(node, noiseValues))
      return;

    // Generate normal data from heightmap
    BlockArrayRect<Vec3> noiseNormals = calcNoiseNormals(node, noiseValues);

    generatePrimaryMesh(node, noiseValues, noiseNormals);
    generateTransitionMeshes(node, noiseValues, noiseNormals);

    node->data->needsUpdate = true;
  }

  // Formulas can be found in section 4.4 of TransVoxel paper
  static bool isVertexNearFace(bool facingPositiveDir, length_t u, length_t cellLength)
  {
    return facingPositiveDir ? u > cellLength * (c_NumCells - 1) : u < cellLength;
  }
  static length_t vertexAdjustment1D(bool facingPositiveDir, length_t u, length_t cellLength)
  {
    return c_TCFractionalWidth * (facingPositiveDir ? ((c_NumCells - 1) * cellLength - u) : (cellLength - u));
  }
  static Mat3 calcVertexTransform(const Vec3& n)
  {
    return Mat3(1 - n.x * n.x, -n.x * n.y, -n.x * n.z,
      -n.x * n.y, 1 - n.y * n.y, -n.y * n.z,
      -n.x * n.z, -n.y * n.z, 1 - n.z * n.z);
  }

  static void adjustVertex(Vertex& vertex, length_t cellLength, uint8_t transitionFaces)
  {
    Vec3 vertexAdjustment{};
    bool isNearSameResolutionLOD = false;
    for (Direction face : Directions())
    {
      int faceID = static_cast<int>(face);
      int coordID = faceID / 2;
      bool facingPositiveDir = faceID % 2;

      if (isVertexNearFace(facingPositiveDir, vertex.position[coordID], cellLength))
      {
        if (transitionFaces & Engine::Bit(faceID))
          vertexAdjustment[coordID] = vertexAdjustment1D(facingPositiveDir, vertex.position[coordID], cellLength);
        else
        {
          isNearSameResolutionLOD = true;
          break;
        }
      }
    }

    if (!isNearSameResolutionLOD && vertexAdjustment != Vec3(0.0))
    {
      const Vec3& n = vertex.isoNormal;
      Mat3 transform = calcVertexTransform(n);

      vertex.position += transform * vertexAdjustment;
    }
  }

  static std::vector<Vertex> calcAdjustedPrimaryMesh(Octree::Node* node)
  {
    length_t cellLength = node->length() / c_NumCells;

    std::vector<Vertex> LODMesh = node->data->primaryMesh.vertices;

    // Adjust coorindates of boundary cells on primary LOD mesh
    if (node->data->transitionFaces != 0)
      for (Vertex& vertex : LODMesh)
        adjustVertex(vertex, cellLength, node->data->transitionFaces);
    return LODMesh;
  }

  static std::vector<Vertex> calcAdjustedTransitionMesh(Octree::Node* node, Direction face)
  {
    static constexpr length_t small = 128 * std::numeric_limits<length_t>::epsilon();

    int faceID = static_cast<int>(face);
    int coordID = faceID / 2;
    bool facingPositiveDir = faceID % 2;
    length_t cellLength = node->length() / c_NumCells;

    std::vector<Vertex> LODMesh = node->data->transitionMeshes[faceID].vertices;

    // Adjust coorindates of boundary cells on transition mesh
    if (node->data->transitionFaces != 0)
      for (Vertex& vertex : LODMesh)
      {
        // If Vertex is on low-resolution side, skip.  If on high-resolution side, move vertex to LOD face
        if (vertex.position[coordID] < small * node->length() || vertex.position[coordID] > (1.0 - small) * node->length())
          continue;
        else
          vertex.position[coordID] = static_cast<float>(facingPositiveDir ? node->length() : 0.0);

        adjustVertex(vertex, cellLength, node->data->transitionFaces);
      }
    return LODMesh;
  }

  /*
    Checks if an LOD is bordered by lower resolution LOD and updates
    given node with that information.
  */
  static void determineTransitionFaces(Octree& tree, Octree::Node* node)
  {
    const GlobalIndex offsets[6] = { { -1, 0, 0 }, { node->size(), 0, 0 }, { 0, -1, 0 }, { 0, node->size(), 0 }, { 0, 0, -1 }, { 0, 0, node->size() } };
                                //       East               West              North              South               Top               Bottom

    // Determine which faces transition to a lower resolution LOD
    uint8_t transitionFaces = 0;
    for (Direction face : Directions())
    {
      int faceID = static_cast<int>(face);

      Octree::Node* neighbor = tree.findLeaf(node->anchor + offsets[faceID]);
      if (neighbor == nullptr)
        continue;

      if (node->LODLevel() == neighbor->LODLevel())
        continue;
      else if (neighbor->LODLevel() - node->LODLevel() == 1)
        transitionFaces |= Engine::Bit(faceID);
      else if (neighbor->LODLevel() - node->LODLevel() > 1)
        EN_WARN("LOD neighbor is more than one level lower resolution");
    }

    node->data->transitionFaces = transitionFaces;
  }

  void UpdateMesh(Octree& tree, Octree::Node* node)
  {
    EN_PROFILE_FUNCTION();

    determineTransitionFaces(tree, node);

    MeshData& primaryMesh = node->data->primaryMesh;
    Engine::Renderer::UploadMesh(primaryMesh.vertexArray.get(), calcAdjustedPrimaryMesh(node), primaryMesh.indices);

    for (Direction face : Directions())
    {
      int faceID = static_cast<int>(face);

      MeshData& transitionMesh = node->data->transitionMeshes[faceID];
      Engine::Renderer::UploadMesh(transitionMesh.vertexArray.get(), calcAdjustedTransitionMesh(node, face), transitionMesh.indices);
    }

    node->data->needsUpdate = false;
  }

  void MessageNeighbors(Octree& tree, Octree::Node* node)
  {
    const GlobalIndex offsets[6] = { { -1, 0, 0 }, { node->size(), 0, 0 }, { 0, -1, 0 }, { 0, node->size(), 0 }, { 0, 0, -1 }, { 0, 0, node->size() } };
                                //       East               West              North              South               Top               Bottom

    // Tell LOD neighbors to update
    for (Direction direction : Directions())
    {
      int directionID = static_cast<int>(direction);

      // Relabel coordinates, u being the coordinate normal to face
      Axis u = AxisOf(direction);
      Axis v = Cycle(u);
      Axis w = Cycle(v);

      globalIndex_t neighborSize = node->size();
      GlobalIndex neighborIndexBase = node->anchor + offsets[directionID];
      for (globalIndex_t i = 0; i < node->size(); i += neighborSize)
        for (globalIndex_t j = 0; j < node->size(); j += neighborSize)
        {
          GlobalIndex neighborIndex = neighborIndexBase;
          neighborIndex[v] += i;
          neighborIndex[w] += j;

          Octree::Node* neighbor = tree.findLeaf(neighborIndex);
          if (neighbor != nullptr)
          {
            neighbor->data->needsUpdate = true;
            neighborSize = neighbor->size();
          }
        }
    }
  }
}