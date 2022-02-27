#include "GMpch.h"
#include "LOD.h"
#include "Player/Player.h"
#include "Util/Noise.h"
#include "Util/Array2D.h"
#include "Util/TransVoxel.h"

struct NoiseData
{
  Vec3 position;
  Vec3 normal;
};

namespace LOD
{
  // Number of cells in each direction
  static constexpr int s_NumCells = Chunk::Size();

  // Width of a transition cell as a fraction of regular cell width
  static constexpr length_t s_TCFractionalWidth = 0.5f;

  Vec3 Octree::Node::anchorPosition() const
  {
    GlobalIndex relativeIndex = anchor - Player::OriginIndex();
    return Chunk::Length() * static_cast<Vec3>(relativeIndex);
  }

  Octree::Octree()
    : m_Root(Node(nullptr, 0, s_RootNodeAnchor))
  {
    m_Root.data = new Data();
  }

  void Octree::splitNode(Node* node)
  {
    EN_ASSERT(node != nullptr, "Node can't be nullptr!");
    EN_ASSERT(node->isLeaf(), "Node must be a leaf node!");
    EN_ASSERT(node->depth != s_MaxNodeDepth, "Node is already at max depth!");

    const int64_t nodeChildSize = bit(s_MaxNodeDepth - node->depth - 1);

    // Divide node into 8 equal-sized child nodes
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        for (int k = 0; k < 2; ++k)
        {
          int childIndex = i * bit32(2) + j * bit32(1) + k * bit32(0);
          EN_ASSERT(node->children[childIndex] == nullptr, "Child node already exists!");

          GlobalIndex nodeChildAnchor = node->anchor + nodeChildSize * GlobalIndex({ i, j, k });
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
    else if (branch->depth < s_MaxNodeDepth)
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
      int childIndex = i * bit32(2) + j * bit32(1) + k * bit32(0);

      return findLeafPriv(branch->children[childIndex], index);
    }
  }

  bool Intersection(AABB boxA, AABB boxB)
  {
    return boxA.min.i < boxB.max.i && boxA.max.i > boxB.min.i &&
           boxA.min.j < boxB.max.j && boxA.max.j > boxB.min.j &&
           boxA.min.k < boxB.max.k && boxA.max.k > boxB.min.k;
  }

  // LOD smoothness parameter, must be in the range [0.0, 1.0]
  static constexpr float smoothnessLevel(int LODLevel)
  {
#if 1
    return std::min(0.1f * (LODLevel) + 0.3f, 1.0f);
#else
    return 1.0f;
#endif
  }

  // Calculate quantity based on values at samples that compose an edge.  The smoothness parameter S is used to interpolate between 
  // roughest iso-surface (vertex is always chosen at edge midpoint) and the smooth iso-surface interpolation used by Paul Bourke.
  template<typename T>
  static T LODInterpolation(float t, float s, const T& q0, const T& q1)
  {
    return (1 - s) * (q0 + q1) / 2 + s * ((1 - t) * q0 + t * q1);
  }

  static Array2D<length_t> generateNoise(LOD::Octree::Node* node)
  {
    EN_PROFILE_FUNCTION();

    const length_t cellLength = node->length() / s_NumCells;
    const Vec2 LODAnchorXY = Chunk::Length() * static_cast<Vec2>(node->anchor);

    Array2D<length_t> noiseValues = Array2D<length_t>(s_NumCells + 1, s_NumCells + 1);
    for (int i = 0; i < s_NumCells + 1; ++i)
      for (int j = 0; j < s_NumCells + 1; ++j)
      {
        // Sample noise at cell corners
        Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j);
        length_t terrainHeight = Noise::FastTerrainNoise2D(pointXY);
        noiseValues[i][j] = terrainHeight;
      }
    return noiseValues;
  }

  static bool needsMesh(LOD::Octree::Node* node, const Array2D<length_t>& noiseValues)
  {
    const length_t LODFloor = node->anchor.k * Chunk::Length();
    const length_t LODCeiling = LODFloor + node->length();

    // Check if LOD is fully below or above surface, if so, no need to generate mesh
    bool needsMesh = false;
    for (int i = 0; i < s_NumCells + 1; ++i)
      for (int j = 0; j < s_NumCells + 1; ++j)
      {
        length_t terrainHeight = noiseValues[i][j];

        if (LODFloor <= terrainHeight && terrainHeight <= LODCeiling)
        {
          needsMesh = true;
          goto endCheck;
        }
      }
  endCheck:;

    return needsMesh;
  }

  static Array2D<Vec3> calcNoiseNormals(LOD::Octree::Node* node, const Array2D<length_t>& noiseValues)
  {
    const length_t cellLength = node->length() / s_NumCells;
    const Vec2 LODAnchorXY = Chunk::Length() * static_cast<Vec2>(node->anchor);

    // Calculate normals using central differences
    Array2D<Vec3> noiseNormals = Array2D<Vec3>(s_NumCells + 1, s_NumCells + 1);
    for (int i = 0; i < s_NumCells + 1; ++i)
      for (int j = 0; j < s_NumCells + 1; ++j)
      {
        // Noise values in adjacent positions.  L - lower, C - center, U - upper
        length_t fLC, fUC, fCL, fCU;

        if (i == 0)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i - 1, j);
          fLC = Noise::FastTerrainNoise2D(pointXY);
        }
        else
          fLC = noiseValues[i - 1][j];

        if (i == s_NumCells)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i + 1, j);
          fUC = Noise::FastTerrainNoise2D(pointXY);
        }
        else
          fUC = noiseValues[i + 1][j];

        if (j == 0)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j - 1);
          fCL = Noise::FastTerrainNoise2D(pointXY);
        }
        else
          fCL = noiseValues[i][j - 1];

        if (j == s_NumCells)
        {
          Vec2 pointXY = LODAnchorXY + cellLength * Vec2(i, j + 1);
          fCU = Noise::FastTerrainNoise2D(pointXY);
        }
        else
          fCU = noiseValues[i][j + 1];

        Vec2 gradient{};
        gradient.x = (fUC - fLC) / (2 * cellLength);
        gradient.y = (fCU - fCL) / (2 * cellLength);

        Vec3 normal = glm::normalize(Vec3(-gradient, 1));
        noiseNormals[i][j] = normal;
      }
    return noiseNormals;
  }

  static NoiseData interpolateNoiseData(LOD::Octree::Node* node, const Array2D<length_t>& noiseValues, const Array2D<Vec3>& noiseNormals, const BlockIndex& sampleA, const BlockIndex& sampleB, float s)
  {
    const length_t LODFloor = node->anchor.k * Chunk::Length();
    const length_t cellLength = node->length() / s_NumCells;

    // Vertex positions
    const Vec3& posA = static_cast<Vec3>(sampleA) * cellLength;
    const Vec3& posB = static_cast<Vec3>(sampleB) * cellLength;

    length_t zA = LODFloor + sampleA.k * cellLength;
    length_t zB = LODFloor + sampleB.k * cellLength;

    // Isovalues of samples A and B
    length_t tA = noiseValues[sampleA.i][sampleA.j] - zA;
    length_t tB = noiseValues[sampleB.i][sampleB.j] - zB;

    // Fraction of distance along edge vertex should be placed
    length_t t = tA / (tA - tB);

    Vec3 vertexPosition = LODInterpolation(t, s, posA, posB);

    // Estimate isosurface normal using linear interpolation between sample points
    const Vec3& n0 = noiseNormals[sampleA.i][sampleA.j];
    const Vec3& n1 = noiseNormals[sampleB.i][sampleB.j];
    Vec3 isoNormal = LODInterpolation(t, s, n0, n1);

    return { vertexPosition, isoNormal };
  }

  // Generate primary LOD mesh using Marching Cubes algorithm
  static void generatePrimaryMesh(LOD::Octree::Node* node, const Array2D<length_t>& noiseValues, const Array2D<Vec3>& noiseNormals)
  {
    const length_t LODFloor = node->anchor.k * Chunk::Length();
    const length_t cellLength = node->length() / s_NumCells;
    const float smoothness = smoothnessLevel(node->LODLevel());

    std::vector<LOD::Vertex> primaryMeshData{};
    for (int i = 0; i < s_NumCells; ++i)
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

            if (noiseValues[I][J] > Z)
              cellCase |= bit(v);
          }
          if (cellCase == 0 || cellCase == 255)
            continue;

          // Use lookup table to determine which of 15 equivalence classes the cell belongs to
          uint8_t cellEquivClass = regularCellClass[cellCase];
          RegularCellData cellData = regularCellData[cellEquivClass];
          int triangleCount = cellData.getTriangleCount();

          // Loop over all triangles in cell
          for (int tri = 0; tri < triangleCount; ++tri)
          {
            std::array<Vec3, 3> vertexPositions{};  // Local positions of vertices within LOD, measured relative to LOD anchor
            std::array<Vec3, 3> isoNormals{};       // Interpolated normals of noise function

            for (int vert = 0; vert < 3; ++vert)
            {
              // Lookup placement of samples A,B that form the cell edge new vertex lies on
              int edgeIndex = cellData.vertexIndex[3 * tri + vert];
              uint16_t vertexData = regularVertexData[cellCase][edgeIndex];
              uint8_t indexA = vertexData & 0x000F;
              uint8_t indexB = (vertexData & 0x00F0) >> 4;

              // Indices of samples A,B
              BlockIndex sampleA{};
              sampleA.i = indexA & bit(0) ? i + 1 : i;
              sampleA.j = indexA & bit(1) ? j + 1 : j;
              sampleA.k = indexA & bit(2) ? k + 1 : k;
              BlockIndex sampleB{};
              sampleB.i = indexB & bit(0) ? i + 1 : i;
              sampleB.j = indexB & bit(1) ? j + 1 : j;
              sampleB.k = indexB & bit(2) ? k + 1 : k;

              NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, sampleA, sampleB, smoothness);

              vertexPositions[vert] = noiseData.position;
              isoNormals[vert] = noiseData.normal;
            }

            // Calculate light value based on surface normal
            Vec3 surfaceNormal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));
            float lightValue = static_cast<float>((1.0 + surfaceNormal.z) / 2);

            for (int vert = 0; vert < 3; ++vert)
              primaryMeshData.push_back({ vertexPositions[vert], isoNormals[vert], vert, lightValue });
          }
        }
    node->data->meshData = primaryMeshData;
  }

  // Generate transition meshes using Transvoxel algorithm
  static void generateTransitionMeshes(LOD::Octree::Node* node, const Array2D<length_t>& noiseValues, const Array2D<Vec3>& noiseNormals)
  {
    static constexpr Vec3 normals[6] = { { 1, 0, 0}, { -1, 0, 0}, { 0, 1, 0}, { 0, -1, 0}, { 0, 0, 1}, { 0, 0, -1} };
                                    //      East         West        North       South         Top        Bottom

    const length_t LODFloor = node->anchor.k * Chunk::Length();
    const length_t cellLength = node->length() / s_NumCells;
    const length_t transitionCellWidth = s_TCFractionalWidth * cellLength;

    for (BlockFace face : BlockFaceIterator())
    {
      const int faceID = static_cast<int>(face);

      // Relabel coordinates, u being the coordinate normal to face
      const int u = faceID / 2;
      const int v = (u + 1) % 3;
      const int w = (u + 2) % 3;
      const bool facingPositiveDir = faceID % 2 == 0;

      const int uIndex = facingPositiveDir ? s_NumCells : 0;

      // Generate transition meshes using Transvoxel algorithm
      std::vector<LOD::Vertex> transitionMeshData{};
      for (int i = 0; i < s_NumCells; i += 2)
        for (int j = 0; j < s_NumCells; j += 2)
        {
          // Determine which of the 512 cases the cell belongs to
          uint16_t cellCase = 0;
          for (int p = 0; p < 9; ++p)
          {
            // From Figure 4.17 in Transvoxel paper
            static constexpr int indexMapping[9] = { 0, 1, 2, 7, 8, 3, 6, 5, 4 };

            BlockIndex cellVertexIndex{};
            cellVertexIndex[u] = uIndex;
            cellVertexIndex[v] = i + p % 3;
            cellVertexIndex[w] = j + p / 3;

            const int& I = cellVertexIndex.i;
            const int& J = cellVertexIndex.j;
            const int& K = cellVertexIndex.k;
            length_t Z = LODFloor + K * cellLength;

            if (noiseValues[I][J] > Z)
              cellCase |= bit(indexMapping[p]);
          }
          if (cellCase == 0 || cellCase == 511)
            continue;

          // Use lookup table to determine which of 56 equivalence classes the cell belongs to
          uint8_t cellEquivClass = transitionCellClass[cellCase];
          bool reverseWindingOrder = static_cast<bool>(cellEquivClass >> 7) == facingPositiveDir;   // Don't know why, but this works
          TransitionCellData cellData = transitionCellData[cellEquivClass & 0x7F];
          int triangleCount = cellData.getTriangleCount();

          // Loop over all triangles in cell
          for (int tri = 0; tri < triangleCount; ++tri)
          {
            // Local positions of vertices within LOD, measured relative to LOD anchor
            std::array<Vec3, 3> vertexPositions{};
            std::array<Vec3, 3> isoNormals{};

            for (int vert = 0; vert < 3; ++vert)
            {
              int vertexIndex = reverseWindingOrder ? 2 - vert : vert;

              // Lookup indices of vertices A,B of the cell edge that vertex v lies on
              int edgeIndex = cellData.vertexIndex[3 * tri + vertexIndex];
              uint16_t vertexData = transitionVertexData[cellCase][edgeIndex];
              uint8_t indexA = vertexData & 0x000F;
              uint8_t indexB = (vertexData & 0x00F0) >> 4;
              bool isOnLowResSide = indexB > 8;

              static constexpr int indexMapping[13] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 2, 6, 8 };

              // Indices of samples A,B
              BlockIndex sampleA{};
              sampleA[u] = uIndex;
              sampleA[v] = i + indexMapping[indexA] % 3;
              sampleA[w] = j + indexMapping[indexA] / 3;
              BlockIndex sampleB{};
              sampleB[u] = uIndex;
              sampleB[v] = i + indexMapping[indexB] % 3;
              sampleB[w] = j + indexMapping[indexB] / 3;

              // If vertex is on low-resolution side, use smoothness level of low-resolution LOD
              float smoothness = isOnLowResSide ? smoothnessLevel(node->LODLevel() + 1) : smoothnessLevel(node->LODLevel());

              NoiseData noiseData = interpolateNoiseData(node, noiseValues, noiseNormals, sampleA, sampleB, smoothness);
              if (!isOnLowResSide)
                noiseData.position -= transitionCellWidth * normals[faceID];

              vertexPositions[vert] = noiseData.position;
              isoNormals[vert] = noiseData.normal;
            }

            // Calculate light value based on surface normal
            Vec3 surfaceNormal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));
            float lightValue = static_cast<float>((1.0 + surfaceNormal.z) / 2);

            for (int vert = 0; vert < 3; ++vert)
              transitionMeshData.push_back({ vertexPositions[vert], isoNormals[vert], vert, lightValue });
          }
        }

      node->data->transitionMeshData[faceID] = transitionMeshData;
    }
  }

  void GenerateMesh(LOD::Octree::Node* node)
  {
    // NOTE: These values should come from biome system when implemented
    static constexpr length_t globalMinTerrainHeight = -205 * Block::Length();
    static constexpr length_t globalMaxTerrainHeight =  205 * Block::Length();

    const length_t LODFloor = node->anchor.k * Chunk::Length();
    const length_t LODCeiling = LODFloor + node->length();

    node->data->meshGenerated = true;

    // If LOD is fully below or above global min/max values, no need to generate mesh
    if (LODFloor > globalMaxTerrainHeight || LODCeiling < globalMinTerrainHeight)
      return;

    EN_PROFILE_FUNCTION();

    // Generate voxel data using heightmap
    const Array2D<length_t> noiseValues = generateNoise(node);

    if (!needsMesh(node, noiseValues))
      return;

    // Generate normal data from heightmap
    const Array2D<Vec3> noiseNormals = calcNoiseNormals(node, noiseValues);

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
  static bool calcVertexAdjustment(Vec3& vertexAdjustment, const Vec3& vertexPosition, length_t cellLength, uint8_t transitionFaces)
  {
    bool isNearSameResolutionLOD = false;
    for (BlockFace face : BlockFaceIterator())
    {
      const int faceID = static_cast<int>(face);
      const int coordID = faceID / 2;
      const bool facingPositiveDir = faceID % 2 == 0;

      if (isVertexNearFace(facingPositiveDir, vertexPosition[coordID], cellLength))
      {
        if (transitionFaces & bit(faceID))
          vertexAdjustment[coordID] = vertexAdjustment1D(facingPositiveDir, vertexPosition[coordID], cellLength);
        else
        {
          isNearSameResolutionLOD = true;
          break;
        }
      }
    }
    return isNearSameResolutionLOD;
  }
  static bool adjustVertex(LOD::Vertex& vertex, length_t cellLength, uint8_t transitionFaces)
  {
    bool vertexAdjusted = false;

    Vec3 adjustment{};
    bool isNearSameResolutionLOD = calcVertexAdjustment(adjustment, vertex.position, cellLength, transitionFaces);

    if (!isNearSameResolutionLOD && adjustment != Vec3(0.0))
    {
      const Vec3& n = vertex.isoNormal;
      const Mat3 transform = calcVertexTransform(n);

      vertex.position += transform * adjustment;
      vertexAdjusted = true;
    }
    return vertexAdjusted;
  }

  std::vector<LOD::Vertex> calcAdjustedPrimaryMesh(LOD::Octree::Node* node)
  {
    const length_t cellLength = node->length() / s_NumCells;

    std::vector<LOD::Vertex> LODMesh = node->data->meshData;

    // Adjust coorindates of boundary cells on primary LOD mesh
    if (node->data->transitionFaces != 0)
      for (auto it = LODMesh.begin(); it != LODMesh.end(); ++it)
        adjustVertex(*it, cellLength, node->data->transitionFaces);
    return LODMesh;
  }

  std::vector<LOD::Vertex> calcAdjustedTransitionMesh(LOD::Octree::Node* node, BlockFace face)
  {
    const int faceID = static_cast<int>(face);
    const int coordID = faceID / 2;
    const bool facingPositiveDir = faceID % 2 == 0;
    const length_t cellLength = node->length() / s_NumCells;

    std::vector<LOD::Vertex> LODMesh = node->data->transitionMeshData[faceID];

    // Adjust coorindates of boundary cells on transition mesh
    if (node->data->transitionFaces != 0)
    {
      int numTriangles = static_cast<int>(LODMesh.size()) / 3;
      for (int tri = 0; tri < numTriangles; ++tri)
      {
        bool triangleModified = false;
        for (int vert = 0; vert < 3; ++vert)
        {
          int i = 3 * tri + vert;

          // If Vertex is on low-resolution side, skip.  If on high-resolution side, move vertex to LOD face
          if (LODMesh[i].position[coordID] < LNGTH_EPSILON * node->length() || LODMesh[i].position[coordID] > (1.0 - LNGTH_EPSILON) * node->length())
            continue;
          else
            LODMesh[i].position[coordID] = facingPositiveDir ? node->length() : static_cast<length_t>(0.0);

          bool vertexAdjusted = adjustVertex(LODMesh[i], cellLength, node->data->transitionFaces);
          if (vertexAdjusted)
            triangleModified = true;
        }

        // Recalculate surface normals
        if (triangleModified)
        {
          int v0 = 3 * tri, v1 = v0 + 1, v2 = v0 + 2;
          std::array<Vec3, 3> vertexPositions = { LODMesh[v0].position, LODMesh[v1].position , LODMesh[v2].position };

          Vec3 surfaceNormal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));
          float lightValue = static_cast<float>((1.0 + surfaceNormal.z) / 2);

          for (int vert = 0; vert < 3; ++vert)
          {
            int i = 3 * tri + vert;
            LODMesh[i].lightValue = lightValue;
          }
        }
      }
    }
    return LODMesh;
  }

  void DetermineTransitionFaces(LOD::Octree& tree, LOD::Octree::Node* node)
  {
    const GlobalIndex offsets[6] = { { node->size(), 0, 0 }, { -1, 0, 0 }, { 0, node->size(), 0 }, { 0, -1, 0 }, { 0, 0, node->size() }, { 0, 0, -1 } };
                                //            East               West              North              South              Top                Bottom

    // Determine which faces transition to a lower resolution LOD
    uint8_t transitionFaces = 0;
    for (BlockFace face : BlockFaceIterator())
    {
      const int faceID = static_cast<int>(face);

      LOD::Octree::Node* neighbor = tree.findLeaf(node->anchor + offsets[faceID]);
      if (neighbor == nullptr)
        continue;

      if (node->LODLevel() == neighbor->LODLevel())
        continue;
      else if (neighbor->LODLevel() - node->LODLevel() == 1)
        transitionFaces |= bit(faceID);
      else if (neighbor->LODLevel() - node->LODLevel() > 1)
        EN_WARN("LOD neighbor is more than one level higher resolution");
    }

    if (node->data->transitionFaces != transitionFaces)
      node->data->needsUpdate = true;

    node->data->transitionFaces = transitionFaces;
  }

  static void uploadMesh(Engine::Shared<Engine::VertexArray>& target, const std::vector<LOD::Vertex>& mesh)
  {
    EN_PROFILE_FUNCTION();

    static const Engine::BufferLayout LODBufferLayout = { { ShaderDataType::Float3, "a_Position" },
                                                          { ShaderDataType::Float3, "a_IsoNormal"},
                                                          { ShaderDataType::Int,    "a_QuadIndex"},
                                                          { ShaderDataType::Float,  "s_LightValue"} };

    // Generate vertex array
    Engine::Shared<Engine::VertexArray> LODVertexArray = Engine::VertexArray::Create();
    auto LODVertexBuffer = Engine::VertexBuffer::Create(static_cast<uint32_t>(mesh.size()) * sizeof(LOD::Vertex));
    LODVertexBuffer->setLayout(LODBufferLayout);
    LODVertexArray->addVertexBuffer(LODVertexBuffer);

    uintptr_t dataSize = sizeof(LOD::Vertex) * mesh.size();
    LODVertexBuffer->setData(mesh.data(), dataSize);
    target = LODVertexArray;
  }

  void UpdateMesh(LOD::Octree& tree, LOD::Octree::Node* node)
  {
    EN_PROFILE_FUNCTION();

    if (node->data->meshData.size() == 0)
      return;

    DetermineTransitionFaces(tree, node);

    if (!node->data->needsUpdate)
      return;

    uploadMesh(node->data->vertexArray, calcAdjustedPrimaryMesh(node));

    for (BlockFace face : BlockFaceIterator())
    {
      const int faceID = static_cast<int>(face);
      uploadMesh(node->data->transitionVertexArrays[faceID], calcAdjustedTransitionMesh(node, face));
    }

    node->data->needsUpdate = false;
  }
}
