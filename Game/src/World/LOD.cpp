#include "GMpch.h"
#include "LOD.h"
#include "Terrain.h"
#include "Player/Player.h"
#include "Util/MultiDimArrays.h"
#include "Util/TransVoxel.h"

// Number of cells in each direction
static constexpr int s_NumCells = Chunk::Size();

// Width of a transition cell as a fraction of regular cell width
static constexpr length_t s_TCFractionalWidth = 0.5f;

static const Biome s_DefaultBiome = Biome::Get(Biome::Type::Default);

struct NoiseData
{
  Vec3 position;
  Vec3 normal;

  Terrain::CompoundSurfaceData surfaceData;
};

Vec3 LOD::Octree::Node::anchorPosition() const
{
  GlobalIndex relativeIndex = anchor - Player::OriginIndex();
  return Chunk::Length() * static_cast<Vec3>(relativeIndex);
}

LOD::Octree::Octree()
  : m_Root(Node(nullptr, 0, s_RootNodeAnchor))
{
  m_Root.data = new Data();
}

void LOD::Octree::splitNode(Node* node)
{
  EN_ASSERT(node != nullptr, "Node can't be nullptr!");
  EN_ASSERT(node->isLeaf(), "Node must be a leaf node!");
  EN_ASSERT(node->depth != s_MaxNodeDepth, "Node is already at max depth!");

  const globalIndex_t nodeChildSize = node->size() / 2;

  // Divide node into 8 equal-sized child nodes
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
      {
        int childIndex = i * bitUi32(2) + j * bitUi32(1) + k * bitUi32(0);
        EN_ASSERT(node->children[childIndex] == nullptr, "Child node already exists!");

        GlobalIndex nodeChildAnchor = node->anchor + nodeChildSize * GlobalIndex(i, j, k);
        node->children[childIndex] = new Node(node, node->depth + 1, nodeChildAnchor);
        node->children[childIndex]->data = new Data();
      }

  // Delete node data as it is no longer a leaf node
  delete node->data;
  node->data = nullptr;
}

void LOD::Octree::combineChildren(Node* node)
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

std::vector<LOD::Octree::Node*> LOD::Octree::getLeaves()
{
  // Recursively collect leaf nodes
  std::vector<Node*> leaves{};
  getLeavesPriv(&m_Root, leaves);

  return leaves;
}

LOD::Octree::Node* LOD::Octree::findLeaf(const GlobalIndex& index)
{
  if (index.i < m_Root.anchor.i || index.i >= m_Root.anchor.i + m_Root.size() ||
      index.j < m_Root.anchor.j || index.j >= m_Root.anchor.j + m_Root.size() ||
      index.k < m_Root.anchor.k || index.k >= m_Root.anchor.k + m_Root.size())
  {
    return nullptr;
  }

  return findLeafPriv(&m_Root, index);
}



void LOD::Octree::getLeavesPriv(Node* branch, std::vector<Node*>& leaves)
{
  if (branch->isLeaf())
    leaves.push_back(branch);
  else if (branch->depth < s_MaxNodeDepth)
    for (int i = 0; i < 8; ++i)
      if (branch->children[i] != nullptr)
        getLeavesPriv(branch->children[i], leaves);
}

LOD::Octree::Node* LOD::Octree::findLeafPriv(Node* branch, const GlobalIndex& index)
{
  if (branch->isLeaf())
    return branch;
  else
  {
    int i = index.i >= branch->anchor.i + branch->size() / 2;
    int j = index.j >= branch->anchor.j + branch->size() / 2;
    int k = index.k >= branch->anchor.k + branch->size() / 2;
    int childIndex = i * bitUi32(2) + j * bitUi32(1) + k * bitUi32(0);

    return findLeafPriv(branch->children[childIndex], index);
  }
}



void LOD::MeshData::Initialize(const Shared<Engine::TextureArray>& textureArray)
{
  s_Shader = Engine::Shader::Create("assets/shaders/ChunkLOD.glsl");
  s_TextureArray = textureArray;
  Engine::UniformBuffer::Allocate(s_UniformBinding, sizeof(LOD::Uniforms));
}

void LOD::MeshData::BindBuffers()
{
  s_Shader->bind();
  Engine::UniformBuffer::Bind(s_UniformBinding);
  s_TextureArray->bind(s_TextureSlot);
}

void LOD::MeshData::SetUniforms(const Uniforms& uniforms)
{
  Engine::UniformBuffer::SetData(s_UniformBinding, &uniforms);
}

void LOD::Draw(const Octree::Node* leaf)
{
  uint32_t primaryMeshIndexCount = static_cast<uint32_t>(leaf->data->primaryMesh.indices.size());

  if (primaryMeshIndexCount == 0)
    return; // Nothing to draw

  // Set local anchor position and texture scaling
  LOD::Uniforms uniforms{};
  uniforms.anchor = Chunk::Length() * static_cast<Vec3>(leaf->anchor - Player::OriginIndex());
  uniforms.textureScaling = static_cast<float>(bit(leaf->LODLevel()));
  MeshData::SetUniforms(uniforms);

  Engine::RenderCommand::DrawIndexed(leaf->data->primaryMesh.vertexArray.get(), primaryMeshIndexCount);
  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);

    uint32_t transitionMeshIndexCount = static_cast<uint32_t>(leaf->data->transitionMeshes[faceID].indices.size());

    if (transitionMeshIndexCount == 0 || !(leaf->data->transitionFaces & bit(faceID)))
      continue;

    Engine::RenderCommand::DrawIndexed(leaf->data->transitionMeshes[faceID].vertexArray.get(), transitionMeshIndexCount);
  }
}

bool LOD::Intersection(AABB boxA, AABB boxB)
{
  return boxA.min.i < boxB.max.i && boxA.max.i > boxB.min.i &&
         boxA.min.j < boxB.max.j && boxA.max.j > boxB.min.j &&
         boxA.min.k < boxB.max.k && boxA.max.k > boxB.min.k;
}



// LOD smoothness parameter, must be in the range [0.0, 1.0]
static constexpr float smoothnessLevel(int LODLevel)
{
#if 1
    return std::min(0.15f * (LODLevel) + 0.3f, 1.0f);
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

static HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1> generateNoise(LOD::Octree::Node* node)
{
  EN_PROFILE_FUNCTION();

  length_t cellLength = node->length() / s_NumCells;
  Vec2 LODAnchorXY = Chunk::Length() * static_cast<Vec2>(node->anchor);

  HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1> noiseValues{};
  for (int i = 0; i < s_NumCells + 1; ++i)
    for (int j = 0; j < s_NumCells + 1; ++j)
    {
      // Sample noise at cell corners
      Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j);
      Noise::OctaveNoiseData<Biome::NumOctaves()> elevationData = Terrain::GetElevationData(pointXY, s_DefaultBiome);
      float seaLevelTemperature = Terrain::GetTemperatureData(pointXY, s_DefaultBiome);
      noiseValues[i][j] = Terrain::GetSurfaceData(elevationData, seaLevelTemperature, s_DefaultBiome);
    }  return noiseValues;
}

static bool needsMesh(LOD::Octree::Node* node, const HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1>& noiseValues)
{
  length_t LODFloor = node->anchor.k * Chunk::Length();
  length_t LODCeiling = LODFloor + node->length();

  // Check if LOD is fully below or above surface, if so, no need to generate mesh
  bool needsMesh = false;
  for (int i = 0; i < s_NumCells + 1; ++i)
    for (int j = 0; j < s_NumCells + 1; ++j)
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

static HeapArray2D<Vec3, s_NumCells + 1> calcNoiseNormals(LOD::Octree::Node* node, const HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1>& noiseValues)
{
  length_t cellLength = node->length() / s_NumCells;
  Vec2 LODAnchorXY = Chunk::Length() * static_cast<Vec2>(node->anchor);

  // Calculate normals using central differences
  HeapArray2D<Vec3, s_NumCells + 1> noiseNormals{};
  for (int i = 0; i < s_NumCells + 1; ++i)
    for (int j = 0; j < s_NumCells + 1; ++j)
    {
      // Surface heights in adjacent positions.  L - lower, C - center, U - upper
      length_t fLC, fUC, fCL, fCU;

      if (i == 0)
      {
        Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i - 1, j);
        fLC = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
      }
      else
        fLC = noiseValues[i - 1][j].getElevation();

      if (i == s_NumCells)
      {
        Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i + 1, j);
        fUC = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
      }
      else
        fUC = noiseValues[i + 1][j].getElevation();

      if (j == 0)
      {
        Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j - 1);
        fCL = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
      }
      else
        fCL = noiseValues[i][j - 1].getElevation();

      if (j == s_NumCells)
      {
        Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j + 1);
        fCU = Terrain::GetElevationData(pointXY, s_DefaultBiome).sum();
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

static NoiseData interpolateNoiseData(LOD::Octree::Node* node, const HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1>& noiseValues, const HeapArray2D<Vec3, s_NumCells + 1>& noiseNormals, const BlockIndex& cornerA, const BlockIndex& cornerB, float s)
{
  length_t LODFloor = node->anchor.k * Chunk::Length();
  length_t cellLength = node->length() / s_NumCells;

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
static void generatePrimaryMesh(LOD::Octree::Node* node, const HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1>& noiseValues, const HeapArray2D<Vec3, s_NumCells + 1>& noiseNormals)
{
  EN_PROFILE_FUNCTION();

  struct VertexReuseData
  {
    uint32_t baseMeshIndex = 0;
    int8_t vertexOrder[4] = { -1, -1, -1, -1 };
  };

  length_t LODFloor = node->anchor.k * Chunk::Length();
  length_t cellLength = node->length() / s_NumCells;
  float smoothness = smoothnessLevel(node->LODLevel());

  int vertexCount = 0;
  std::vector<uint32_t> primaryMeshIndices{};
  std::vector<LOD::Vertex> primaryMeshVertices{};
  HeapArray2D<VertexReuseData, s_NumCells> prevLayer{};
  for (int i = 0; i < s_NumCells; ++i)
  {
    HeapArray2D<VertexReuseData, s_NumCells> currLayer{};

    for (int j = 0; j < s_NumCells; ++j)
      for (int k = 0; k < s_NumCells; ++k)
      {
        // Determine which of the 256 cases the cell belongs to
        uint8_t cellCase = 0;
        for (int v = 0; v < 8; ++v)
        {
          // Cell corner indices and z-position
          int I = v & bit(0) ? i + 1 : i;
          int J = v & bit(1) ? j + 1 : j;
          int K = v & bit(2) ? k + 1 : k;
          length_t Z = LODFloor + K * cellLength;

          if (noiseValues[I][J].getElevation() > Z)
            cellCase |= bit(v);
        }
        if (cellCase == 0 || cellCase == 255)
          continue;

        currLayer[j][k].baseMeshIndex = vertexCount;

        // Use lookup table to determine which of 15 equivalence classes the cell belongs to
        uint8_t cellEquivClass = regularCellClass[cellCase];
        RegularCellData cellData = regularCellData[cellEquivClass];
        int triangleCount = cellData.getTriangleCount();

        // Loop over all triangles in cell
        int cellVertexCount = 0;
        std::array<uint32_t, maxCellVertexCount> prevCellVertexIndices{};
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
          uint16_t vertexData = regularVertexData[cellCase][edgeIndex];
          uint8_t sharedVertexIndex = (vertexData & 0x0F00) >> 8;
          uint8_t sharedVertexDirection = (vertexData & 0xF000) >> 12;
          bool newVertex = sharedVertexDirection == 8;

          // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
          if (newVertex)
            currLayer[j][k].vertexOrder[sharedVertexIndex] = cellVertexCount;
          else
          {
            int I = sharedVertexDirection & bit(0) ? i - 1 : i;
            int J = sharedVertexDirection & bit(1) ? j - 1 : j;
            int K = sharedVertexDirection & bit(2) ? k - 1 : k;

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

          uint8_t cornerIndexA =  vertexData & 0x000F;
          uint8_t cornerIndexB = (vertexData & 0x00F0) >> 4;

          // Indices of corners A,B
          BlockIndex cornerA{};
          cornerA.i = cornerIndexA & bit(0) ? i + 1 : i;
          cornerA.j = cornerIndexA & bit(1) ? j + 1 : j;
          cornerA.k = cornerIndexA & bit(2) ? k + 1 : k;
          BlockIndex cornerB{};
          cornerB.i = cornerIndexB & bit(0) ? i + 1 : i;
          cornerB.j = cornerIndexB & bit(1) ? j + 1 : j;
          cornerB.k = cornerIndexB & bit(2) ? k + 1 : k;

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
static void generateTransitionMeshes(LOD::Octree::Node* node, const HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1>& noiseValues, const HeapArray2D<Vec3, s_NumCells + 1>& noiseNormals)
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
  length_t cellLength = node->length() / s_NumCells;
  length_t transitionCellWidth = s_TCFractionalWidth * cellLength;

  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);

    // Relabel coordinates, u being the coordinate normal to face
    int u = faceID / 2;
    int v = (u + 1) % 3;
    int w = (u + 2) % 3;
    int uIndex = IsPositive(face) ? s_NumCells : 0;

    // Generate transition mesh using Transvoxel algorithm
    int vertexCount = 0;
    std::vector<uint32_t> transitionMeshIndices{};
    std::vector<LOD::Vertex> transitionMeshVertices{};
    std::array<VertexReuseData, s_NumCells / 2> prevRow{};
    for (int i = 0; i < s_NumCells; i += 2)
    {
      std::array<VertexReuseData, s_NumCells / 2> currRow{};

      for (int j = 0; j < s_NumCells; j += 2)
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
          if (!IsPositive(face))
            sample[v] = s_NumCells - sample[v];

          const int& I = sample.i;
          const int& J = sample.j;
          const int& K = sample.k;
          length_t Z = LODFloor + K * cellLength;

          if (noiseValues[I][J].getElevation() > Z)
            cellCase |= bit(sampleIndexToBitFlip[p]);
        }
        if (cellCase == 0 || cellCase == 511)
          continue;

        currRow[j / 2].baseMeshIndex = vertexCount;

        // Use lookup table to determine which of 56 equivalence classes the cell belongs to
        uint8_t cellEquivClass = transitionCellClass[cellCase];
        bool reverseWindingOrder = cellEquivClass >> 7;
        TransitionCellData cellData = transitionCellData[cellEquivClass & 0x7F];
        int triangleCount = cellData.getTriangleCount();

        // Loop over all triangles in cell
        int cellVertexCount = 0;
        std::array<uint32_t, maxCellVertexCount> prevCellVertexIndices{};
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
          uint16_t vertexData = transitionVertexData[cellCase][edgeIndex];
          uint8_t sharedVertexIndex = (vertexData & 0x0F00) >> 8;
          uint8_t sharedVertexDirection = (vertexData & 0xF000) >> 12;
          bool isReusable = sharedVertexDirection != 4;
          bool newVertex = sharedVertexDirection == 8;

          // If a new vertex must be created, store vertex index information for later reuse.  If not, attempt to reuse previous vertex
          if (newVertex)
            currRow[j / 2].vertexOrder[sharedVertexIndex] = cellVertexCount;
          else if (isReusable)
          {
            int I = sharedVertexDirection & bit(0) ? i / 2 - 1 : i / 2;
            int J = sharedVertexDirection & bit(1) ? j / 2 - 1 : j / 2;

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
          sampleA[v] = i + cornerIndexToSampleIndex[cornerIndexA] % 3;
          sampleA[w] = j + cornerIndexToSampleIndex[cornerIndexA] / 3;
          BlockIndex sampleB{};
          sampleB[u] = uIndex;
          sampleB[v] = i + cornerIndexToSampleIndex[cornerIndexB] % 3;
          sampleB[w] = j + cornerIndexToSampleIndex[cornerIndexB] / 3;

          // If block face normal along negative axis, we must reverse v coordinate-axis to ensure correct orientation
          if (!IsPositive(face))
          {
            sampleA[v] = s_NumCells - sampleA[v];
            sampleB[v] = s_NumCells - sampleB[v];
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

void LOD::GenerateMesh(LOD::Octree::Node* node)
{
  // NOTE: These values should come from biome system when implemented
  static const length_t globalMinTerrainHeight = s_DefaultBiome.minElevation();
  static const length_t globalMaxTerrainHeight = s_DefaultBiome.maxElevation();

  length_t LODFloor = node->anchor.k * Chunk::Length();
  length_t LODCeiling = LODFloor + node->length();

  node->data->meshGenerated = true;

  // If LOD is fully below or above global min/max values, no need to generate mesh
  if (LODFloor > globalMaxTerrainHeight || LODCeiling < globalMinTerrainHeight)
    return;

  EN_PROFILE_FUNCTION();

  // Generate voxel data using heightmap
  HeapArray2D<Terrain::CompoundSurfaceData, s_NumCells + 1> noiseValues = generateNoise(node);

  if (!needsMesh(node, noiseValues))
    return;

  // Generate normal data from heightmap
  HeapArray2D<Vec3, s_NumCells + 1> noiseNormals = calcNoiseNormals(node, noiseValues);

  generatePrimaryMesh(node, noiseValues, noiseNormals);
  generateTransitionMeshes(node, noiseValues, noiseNormals);

  node->data->needsUpdate = true;
}

// Formulas can be found in section 4.4 of TransVoxel paper
static bool isVertexNearFace(bool facingPositiveDir, length_t u, length_t cellLength)
{
  return facingPositiveDir ? u > cellLength * (s_NumCells - 1) : u < cellLength;
}
static length_t vertexAdjustment1D(bool facingPositiveDir, length_t u, length_t cellLength)
{
  return s_TCFractionalWidth * (facingPositiveDir ? ((s_NumCells - 1) * cellLength - u) : (cellLength - u));
}
static Mat3 calcVertexTransform(const Vec3& n)
{
  return Mat3(  1 - n.x * n.x,  -n.x * n.y,      -n.x * n.z,
               -n.x * n.y,       1 - n.y * n.y,  -n.y * n.z,
               -n.x * n.z,      -n.y * n.z,       1 - n.z * n.z);
}

static void adjustVertex(LOD::Vertex& vertex, length_t cellLength, uint8_t transitionFaces)
{
  Vec3 vertexAdjustment{};
  bool isNearSameResolutionLOD = false;
  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);
    int coordID = faceID / 2;
    bool facingPositiveDir = faceID % 2;

    if (isVertexNearFace(facingPositiveDir, vertex.position[coordID], cellLength))
    {
      if (transitionFaces & bit(faceID))
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

static std::vector<LOD::Vertex> calcAdjustedPrimaryMesh(LOD::Octree::Node* node)
{
  length_t cellLength = node->length() / s_NumCells;

  std::vector<LOD::Vertex> LODMesh = node->data->primaryMesh.vertices;

  // Adjust coorindates of boundary cells on primary LOD mesh
  if (node->data->transitionFaces != 0)
    for (LOD::Vertex& vertex : LODMesh)
      adjustVertex(vertex, cellLength, node->data->transitionFaces);
  return LODMesh;
}

static std::vector<LOD::Vertex> calcAdjustedTransitionMesh(LOD::Octree::Node* node, Block::Face face)
{
  int faceID = static_cast<int>(face);
  int coordID = faceID / 2;
  bool facingPositiveDir = faceID % 2;
  length_t cellLength = node->length() / s_NumCells;

  std::vector<LOD::Vertex> LODMesh = node->data->transitionMeshes[faceID].vertices;

  // Adjust coorindates of boundary cells on transition mesh
  if (node->data->transitionFaces != 0)
    for (LOD::Vertex& vertex : LODMesh)
    {
      // If Vertex is on low-resolution side, skip.  If on high-resolution side, move vertex to LOD face
      if (vertex.position[coordID] < Constants::LENGTH_EPSILON * node->length() || vertex.position[coordID] > (1.0 - Constants::LENGTH_EPSILON) * node->length())
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
static void determineTransitionFaces(LOD::Octree& tree, LOD::Octree::Node* node)
{
  const GlobalIndex offsets[6] = { { -1, 0, 0 }, { node->size(), 0, 0 }, { 0, -1, 0 }, { 0, node->size(), 0 }, { 0, 0, -1 }, { 0, 0, node->size() } };
                              //       East               West              North              South               Top               Bottom

  // Determine which faces transition to a lower resolution LOD
  uint8_t transitionFaces = 0;
  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);

    LOD::Octree::Node* neighbor = tree.findLeaf(node->anchor + offsets[faceID]);
    if (neighbor == nullptr)
      continue;

    if (node->LODLevel() == neighbor->LODLevel())
      continue;
    else if (neighbor->LODLevel() - node->LODLevel() == 1)
      transitionFaces |= bit(faceID);
    else if (neighbor->LODLevel() - node->LODLevel() > 1)
      EN_WARN("LOD neighbor is more than one level lower resolution");
  }

  node->data->transitionFaces = transitionFaces;
}

void LOD::UpdateMesh(LOD::Octree& tree, LOD::Octree::Node* node)
{
  EN_PROFILE_FUNCTION();

  determineTransitionFaces(tree, node);

  LOD::MeshData& primaryMesh = node->data->primaryMesh;
  Engine::Renderer::UploadMesh(primaryMesh.vertexArray.get(), calcAdjustedPrimaryMesh(node), primaryMesh.indices);

  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);

    LOD::MeshData& transitionMesh = node->data->transitionMeshes[faceID];
    Engine::Renderer::UploadMesh(transitionMesh.vertexArray.get(), calcAdjustedTransitionMesh(node, face), transitionMesh.indices);
  }

  node->data->needsUpdate = false;
}

void LOD::MessageNeighbors(LOD::Octree& tree, LOD::Octree::Node* node)
{
  const GlobalIndex offsets[6] = { { -1, 0, 0 }, { node->size(), 0, 0 }, { 0, -1, 0 }, { 0, node->size(), 0 }, { 0, 0, -1 }, { 0, 0, node->size() } };
                              //       East               West              North              South               Top               Bottom

  // Tell LOD neighbors to update
  for (Block::Face face : Block::FaceIterator())
  {
    int faceID = static_cast<int>(face);
    int oppFaceID = static_cast<int>(!face);

    // Relabel coordinates, u being the coordinate normal to face
    int u = faceID / 2;
    int v = (u + 1) % 3;
    int w = (u + 2) % 3;

    globalIndex_t neighborSize = node->size();
    GlobalIndex neighborIndexBase = node->anchor + offsets[faceID];
    for (globalIndex_t i = 0; i < node->size(); i += neighborSize)
      for (globalIndex_t j = 0; j < node->size(); j += neighborSize)
      {
        GlobalIndex neighborIndex = neighborIndexBase;
        neighborIndex[v] += i;
        neighborIndex[w] += j;

        LOD::Octree::Node* neighbor = tree.findLeaf(neighborIndex);
        if (neighbor != nullptr)
        {
          neighbor->data->needsUpdate = true;
          neighborSize = neighbor->size();
        }
      }
  }
}