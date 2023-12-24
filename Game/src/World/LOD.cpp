#include "GMpch.h"
#include "LOD.h"
#include "Terrain.h"
#include "Player/Player.h"
#include "Util/TransVoxel.h"

namespace lod
{
  // Number of cells in each direction
  static constexpr i32 c_NumCells = Chunk::Size();

  // Width of a transition cell as a fraction of regular cell width
  static constexpr length_t c_TCFractionalWidth = 0.5f;

  static constexpr BlockBox c_LODBounds(0, c_NumCells);
  static constexpr BlockRect c_LODBounds2D(0, c_NumCells);

  struct NoiseData
  {
    eng::math::Vec3 position;
    eng::math::Vec3 normal;

    terrain::CompoundSurfaceData surfaceData;
  };

  Vertex::Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, const std::array<i32, 2>& textureIndices, const eng::math::Float2& textureWeights, i32 quadIndex)
    : position(position), isoNormal(isoNormal), textureIndices(textureIndices), textureWeights(textureWeights), quadIndex(quadIndex) {}

  MeshData::MeshData()
    : vertices(), indices()
  {
    vertexArray = eng::VertexArray::Create();
    vertexArray->setLayout(s_VertexBufferLayout);
  }

  Octree::Node::Node(Octree::Node* parentNode, i32 nodeDepth, const GlobalIndex& anchorIndex)
    : parent(parentNode), depth(nodeDepth), anchor(anchorIndex) {}

  Octree::Node::~Node()
  {
    delete data;
    data = nullptr;
    for (i32 i = 0; i < 8; ++i)
    {
      delete children[i];
      children[i] = nullptr;
    }
  }

  bool Octree::Node::isRoot() const { return parent == nullptr; }
  bool Octree::Node::isLeaf() const { return data != nullptr; }
  i32 Octree::Node::LODLevel() const { return c_MaxNodeDepth - depth; }

  globalIndex_t Octree::Node::size() const { return eng::math::pow2<globalIndex_t>(LODLevel()); }
  length_t Octree::Node::length() const { return size() * Chunk::Length(); }

  eng::math::Vec3 Octree::Node::anchorPosition() const
  {
    GlobalIndex relativeIndex = anchor - player::originIndex();
    return Chunk::Length() * static_cast<eng::math::Vec3>(relativeIndex);
  }

  eng::math::Vec3 Octree::Node::center() const { return anchorPosition() + length() / 2; }
  GlobalBox Octree::Node::boundingBox() const { return { anchor, anchor + size() }; }




  Octree::Octree()
    : m_Root(Node(nullptr, 0, c_RootNodeAnchor)) { m_Root.data = new Data(); }

  void Octree::splitNode(Node* node)
  {
    ENG_ASSERT(node != nullptr, "Node can't be nullptr!");
    ENG_ASSERT(node->isLeaf(), "Node must be a leaf node!");
    ENG_ASSERT(node->depth != c_MaxNodeDepth, "Node is already at max depth!");

    const globalIndex_t nodeChildSize = node->size() / 2;

    // Divide node into 8 equal-sized child nodes
    for (i32 i = 0; i < 2; ++i)
      for (i32 j = 0; j < 2; ++j)
        for (i32 k = 0; k < 2; ++k)
        {
          i32 childIndex = i * eng::u32Bit(2) + j * eng::u32Bit(1) + k * eng::u32Bit(0);
          ENG_ASSERT(node->children[childIndex] == nullptr, "Child node already exists!");

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
    ENG_ASSERT(node != nullptr, "Node can't be nullptr!");

    // Return if node is already a leaf
    if (node->isLeaf())
      return;

    // Delete child nodes
    for (i32 i = 0; i < 8; ++i)
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
      for (Node* child : branch->children)
        if (child != nullptr)
          getLeavesPriv(child, leaves);
  }

  Octree::Node* Octree::findLeafPriv(Node* branch, const GlobalIndex& index)
  {
    if (branch->isLeaf())
      return branch;
    else
    {
      i32 i = index.i >= branch->anchor.i + branch->size() / 2;
      i32 j = index.j >= branch->anchor.j + branch->size() / 2;
      i32 k = index.k >= branch->anchor.k + branch->size() / 2;
      i32 childIndex = i * eng::u32Bit(2) + j * eng::u32Bit(1) + k * eng::u32Bit(0);

      return findLeafPriv(branch->children[childIndex], index);
    }
  }



  void MeshData::Initialize()
  {
    static bool initialized = []()
    {
      s_Uniform = std::make_unique<eng::Uniform>("LOD", c_UniformBinding, sizeof(UniformData));
      s_Shader = eng::Shader::Create("assets/shaders/ChunkLOD.glsl");
      s_TextureArray = block::getTextureArray();
      return true;
    }();
  }

  void MeshData::BindBuffers()
  {
    Initialize();
    s_TextureArray->bind(c_TextureSlot);
  }

  void MeshData::SetUniforms(const UniformData& uniformData)
  {
    Initialize();
    s_Uniform->write(uniformData);
  }



  void draw(const Octree::Node* leaf)
  {
    u32 primaryMeshIndexCount = eng::arithmeticCast<u32>(leaf->data->primaryMesh.indices.size());

    if (primaryMeshIndexCount == 0)
      return; // Nothing to draw

    // Set local anchor position and texture scaling
    UniformData uniformData{};
    uniformData.anchor = Chunk::Length() * static_cast<eng::math::Vec3>(leaf->anchor - player::originIndex());
    uniformData.textureScaling = eng::arithmeticUpcast<f32>(eng::bit(leaf->LODLevel()));
    MeshData::SetUniforms(uniformData);

    eng::render::command::drawIndexed(*leaf->data->primaryMesh.vertexArray, primaryMeshIndexCount);
    for (eng::math::Direction face : eng::math::Directions())
    {
      u32 transitionMeshIndexCount = eng::arithmeticCast<u32>(leaf->data->transitionMeshes[face].indices.size());
      if (transitionMeshIndexCount == 0 || !(leaf->data->transitionFaces & eng::bit(eng::toUnderlying(face))))
        continue;

      eng::render::command::drawIndexed(*leaf->data->transitionMeshes[face].vertexArray, transitionMeshIndexCount);
    }
  }



  // LOD smoothness parameter, must be in the range [0.0, 1.0]
  static constexpr f32 smoothnessLevel(i32 LODLevel)
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
  static T LODInterpolation(f32 t, f32 s, const T& q0, const T& q1)
  {
    return ((1 - s) / 2 + s * (1 - t)) * q0 + ((1 - s) / 2 + s * t) * q1;
  }

  static BlockArrayRect<terrain::CompoundSurfaceData> generateNoise(Octree::Node* node)
  {
    length_t cellLength = node->length() / c_NumCells;
    eng::math::Vec2 LODAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node->anchor);

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

  static bool needsMesh(Octree::Node* node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t LODCeiling = LODFloor + node->length();

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

  static BlockArrayRect<eng::math::Vec3> calcNoiseNormals(Octree::Node* node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues)
  {
    length_t cellLength = node->length() / c_NumCells;
    eng::math::Vec2 LODAnchorXY = Chunk::Length() * static_cast<eng::math::Vec2>(node->anchor);

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

  static NoiseData interpolateNoiseData(Octree::Node* node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues, const BlockArrayRect<eng::math::Vec3>& noiseNormals, const BlockIndex& cornerA, const BlockIndex& cornerB, f32 s)
  {
    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t cellLength = node->length() / c_NumCells;

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

    eng::math::Vec3 vertexPosition = LODInterpolation(t, s, posA, posB);
    terrain::CompoundSurfaceData surfaceData = LODInterpolation(t, s, surfaceDataA, surfaceDataB);

    // Estimate isosurface normal using linear interpolation between corners
    const eng::math::Vec3& n0 = noiseNormals[cornerA.i][cornerA.j];
    const eng::math::Vec3& n1 = noiseNormals[cornerB.i][cornerB.j];
    eng::math::Vec3 isoNormal = LODInterpolation(t, s, n0, n1);

    return { vertexPosition, isoNormal, surfaceData };
  }

  // Generate primary LOD mesh using Marching Cubes algorithm
  static void generatePrimaryMesh(Octree::Node* node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues, const BlockArrayRect<eng::math::Vec3>& noiseNormals)
  {
    ENG_PROFILE_FUNCTION();

    struct VertexReuseData
    {
      u32 baseMeshIndex = 0;
      i8 vertexOrder[4] = { -1, -1, -1, -1 };
    };
    using VertexLayer = eng::math::ArrayRect<VertexReuseData, blockIndex_t>;

    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t cellLength = node->length() / c_NumCells;
    f32 smoothness = smoothnessLevel(node->LODLevel());

    i32 vertexCount = 0;
    std::vector<u32> primaryMeshIndices{};
    std::vector<Vertex> primaryMeshVertices{};
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
    node->data->primaryMesh.vertices = std::move(primaryMeshVertices);
    node->data->primaryMesh.indices = std::move(primaryMeshIndices);
  }

  // Generate transition meshes using Transvoxel algorithm
  static void generateTransitionMeshes(Octree::Node* node, const BlockArrayRect<terrain::CompoundSurfaceData>& noiseValues, const BlockArrayRect<eng::math::Vec3>& noiseNormals)
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

    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t cellLength = node->length() / c_NumCells;
    length_t transitionCellWidth = c_TCFractionalWidth * cellLength;

    for (eng::math::Direction face : eng::math::Directions())
    {
      // Relabel coordinates, u being the coordinate normal to face
      eng::math::Axis u = axisOf(face);
      eng::math::Axis v = cycle(u);
      eng::math::Axis w = cycle(v);
      i32 uIndex = isUpstream(face) ? c_NumCells : 0;

      // Generate transition mesh using Transvoxel algorithm
      i32 vertexCount = 0;
      std::vector<u32> transitionMeshIndices{};
      std::vector<Vertex> transitionMeshVertices{};
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
            f32 smoothness = isOnLowResSide ? smoothnessLevel(node->LODLevel() + 1) : smoothnessLevel(node->LODLevel());

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

      node->data->transitionMeshes[face].vertices = std::move(transitionMeshVertices);
      node->data->transitionMeshes[face].indices = std::move(transitionMeshIndices);
    }
  }

  void generateMesh(Octree::Node* node)
  {
    // NOTE: These values should come from biome system when implemented
    static const length_t globalMinTerrainHeight = -400 * block::length();
    static const length_t globalMaxTerrainHeight = 400 * block::length();

    length_t LODFloor = node->anchor.k * Chunk::Length();
    length_t LODCeiling = LODFloor + node->length();

    node->data->meshGenerated = true;

    // If LOD is fully below or above global min/max values, no need to generate mesh
    if (LODFloor > globalMaxTerrainHeight || LODCeiling < globalMinTerrainHeight)
      return;

    ENG_PROFILE_FUNCTION();

    // Generate voxel data using heightmap
    BlockArrayRect<terrain::CompoundSurfaceData> noiseValues = generateNoise(node);

    if (!needsMesh(node, noiseValues))
      return;

    // Generate normal data from heightmap
    BlockArrayRect<eng::math::Vec3> noiseNormals = calcNoiseNormals(node, noiseValues);

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
  static eng::math::Mat3 calcVertexTransform(const eng::math::Vec3& n)
  {
    return eng::math::Mat3(1 - n.x * n.x, -n.x * n.y, -n.x * n.z,
                           -n.x * n.y, 1 - n.y * n.y, -n.y * n.z,
                           -n.x * n.z, -n.y * n.z, 1 - n.z * n.z);
  }

  static void adjustVertex(Vertex& vertex, length_t cellLength, u8 transitionFaces)
  {
    eng::math::Vec3 vertexAdjustment{};
    bool isNearSameResolutionLOD = false;
    for (eng::math::Direction face : eng::math::Directions())
    {
      i32 axisID = eng::toUnderlying(eng::math::axisOf(face));
      if (isVertexNearFace(eng::math::isUpstream(face), vertex.position[axisID], cellLength))
      {
        if (transitionFaces & eng::bit(eng::toUnderlying(face)))
          vertexAdjustment[axisID] = vertexAdjustment1D(eng::math::isUpstream(face), vertex.position[axisID], cellLength);
        else
        {
          isNearSameResolutionLOD = true;
          break;
        }
      }
    }

    if (!isNearSameResolutionLOD && vertexAdjustment != eng::math::Vec3(0.0))
    {
      const eng::math::Vec3& n = vertex.isoNormal;
      eng::math::Mat3 transform = calcVertexTransform(n);

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

  static std::vector<Vertex> calcAdjustedTransitionMesh(Octree::Node* node, eng::math::Direction face)
  {
    static constexpr length_t tolerance = 128 * std::numeric_limits<length_t>::epsilon();

    i32 axisID = eng::toUnderlying(eng::math::axisOf(face));
    length_t cellLength = node->length() / c_NumCells;

    std::vector<Vertex> LODMesh = node->data->transitionMeshes[face].vertices;

    // Adjust coorindates of boundary cells on transition mesh
    if (node->data->transitionFaces != 0)
      for (Vertex& vertex : LODMesh)
      {
        // If Vertex is on low-resolution side, skip.  If on high-resolution side, move vertex to LOD face
        if (vertex.position[axisID] < tolerance * node->length() || vertex.position[axisID] > (1.0 - tolerance) * node->length())
          continue;
        else
          vertex.position[axisID] = eng::arithmeticCastUnchecked<f32>(eng::math::isUpstream(face) ? node->length() : 0.0);

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
    const eng::EnumArray<GlobalIndex, eng::math::Direction> offsets =
      { { eng::math::Direction::West,   {-1,            0,            0} },
        { eng::math::Direction::East,   {node->size(),  0,            0} },
        { eng::math::Direction::South,  { 0,           -1,            0} },
        { eng::math::Direction::North,  { 0, node->size(),            0} },
        { eng::math::Direction::Bottom, { 0,            0,           -1} },
        { eng::math::Direction::Top,    { 0,            0, node->size()} } };

    // Determine which faces transition to a lower resolution LOD
    u8 transitionFaces = 0;
    for (eng::math::Direction face : eng::math::Directions())
    {
      Octree::Node* neighbor = tree.findLeaf(node->anchor + offsets[face]);
      if (neighbor == nullptr)
        continue;

      if (node->LODLevel() == neighbor->LODLevel())
        continue;
      else if (neighbor->LODLevel() - node->LODLevel() == 1)
        transitionFaces |= eng::bit(eng::toUnderlying(face));
      else if (neighbor->LODLevel() - node->LODLevel() > 1)
        ENG_WARN("LOD neighbor is more than one level lower resolution");
    }

    node->data->transitionFaces = transitionFaces;
  }

  void updateMesh(Octree& tree, Octree::Node* node)
  {
    ENG_PROFILE_FUNCTION();

    determineTransitionFaces(tree, node);

    MeshData& primaryMesh = node->data->primaryMesh;
    eng::render::uploadMesh(primaryMesh.vertexArray.get(), calcAdjustedPrimaryMesh(node), primaryMesh.indices);

    for (eng::math::Direction face : eng::math::Directions())
    {
      MeshData& transitionMesh = node->data->transitionMeshes[face];
      eng::render::uploadMesh(transitionMesh.vertexArray.get(), calcAdjustedTransitionMesh(node, face), transitionMesh.indices);
    }

    node->data->needsUpdate = false;
  }

  void messageNeighbors(Octree& tree, Octree::Node* node)
  {
    const eng::EnumArray<GlobalIndex, eng::math::Direction> offsets =
      { { eng::math::Direction::West,   {-1,            0,            0} },
        { eng::math::Direction::East,   {node->size(),  0,            0} },
        { eng::math::Direction::South,  { 0,           -1,            0} },
        { eng::math::Direction::North,  { 0, node->size(),            0} },
        { eng::math::Direction::Bottom, { 0,            0,           -1} },
        { eng::math::Direction::Top,    { 0,            0, node->size()} } };

    // Tell LOD neighbors to update
    for (eng::math::Direction direction : eng::math::Directions())
    {
      // Relabel coordinates, u being the coordinate normal to face
      eng::math::Axis u = axisOf(direction);
      eng::math::Axis v = cycle(u);
      eng::math::Axis w = cycle(v);

      globalIndex_t neighborSize = node->size();
      GlobalIndex neighborIndexBase = node->anchor + offsets[direction];
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