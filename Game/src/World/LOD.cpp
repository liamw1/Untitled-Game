#include "GMpch.h"
#include "LOD.h"
#include "Player/Player.h"
#include "Util/Noise.h"
#include "Util/Array2D.h"
#include "Util/TransVoxel.h"

namespace LOD
{
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

  void GenerateMesh(LOD::Octree::Node* node)
  {
    // LOD smoothness parameter, must be in the range [0.0, 1.0]
#if 0
    const float S = std::min(0.2f * node->LODLevel(), 1.0f);
#else
    const float S = 1.0f;
#endif

  // NOTE: These values should come from biome system when implemented
    static constexpr length_t globalMinTerrainHeight = -255 * Block::Length();
    static constexpr length_t globalMaxTerrainHeight = 255 * Block::Length();

    // Number of cells in each direction
    constexpr int numCells = Chunk::Size();

    const length_t cellLength = node->length() / numCells;

    if (node->anchor.k * Chunk::Length() > globalMaxTerrainHeight || node->anchor.k * Chunk::Length() + node->length() < globalMinTerrainHeight)
      return;

    // Generate heightmap
    length_t minTerrainHeight = std::numeric_limits<length_t>::max();
    length_t maxTerrainHeight = -std::numeric_limits<length_t>::max();
    Array2D<Vec4> noiseValues = Array2D<Vec4>(numCells + 3, numCells + 3);
    for (int i = 0; i < numCells + 3; ++i)
      for (int j = 0; j < numCells + 3; ++j)
      {
        Vec2 pointXY = Chunk::Length() * static_cast<Vec2>(node->anchor) + cellLength * Vec2(i - 1, j - 1);
        noiseValues[i][j] = Noise::TerrainNoise2D(pointXY);
        length_t terrainHeight = noiseValues[i][j].w;

        if (terrainHeight > maxTerrainHeight)
          maxTerrainHeight = terrainHeight;
        if (terrainHeight < minTerrainHeight)
          minTerrainHeight = terrainHeight;
      }

    if (node->anchor.k * Chunk::Length() > maxTerrainHeight || node->anchor.k * Chunk::Length() + node->length() < minTerrainHeight)
      return;

    // Generate primary LOD mesh
    std::vector<LOD::Vertex> LODMeshData{};
    for (int i = 1; i < numCells + 1; ++i)
      for (int j = 1; j < numCells + 1; ++j)
        for (int k = 1; k < numCells + 1; ++k)
        {
          const length_t cellAnchorZPos = node->anchor.k * Chunk::Length() + (k - 1) * cellLength;

          // Determine which of the 256 cases the cell belongs to
          uint8_t cellCase = 0;
          for (int v = 0; v < 8; ++v)
          {
            // Cell vertex indices and z-position
            int I = v & bit(0) ? i + 1 : i;
            int J = v & bit(1) ? j + 1 : j;
            length_t Z = v & bit(2) ? cellAnchorZPos + cellLength : cellAnchorZPos;

            if (noiseValues[I][J].w > Z)
              cellCase |= bit(v);
          }
          if (cellCase == 0 || cellCase == 255)
            continue;

          // Use lookup table to determine which of 16 equivalence classes the cell belongs to
          uint8_t cellEquivClass = regularCellClass[cellCase];
          RegularCellData cellData = regularCellData[cellEquivClass];
          int triangleCount = cellData.getTriangleCount();

          // Loop over all triangles in cell
          for (int tri = 0; tri < triangleCount; ++tri)
          {
            // Local positions of vertices within LOD, measured relative to LOD anchor
            std::array<Vec3, 3> vertexPositions{};
            std::array<Vec3, 3> isoNormals{};

            for (int vert = 0; vert < 3; ++vert)
            {
              // Lookup indices of vertices A,B of the cell edge that vertex v lies on
              int edgeIndex = cellData.vertexIndex[3 * tri + vert];
              uint16_t vertexData = regularVertexData[cellCase][edgeIndex];
              uint8_t indexA = vertexData & 0x000F;
              uint8_t indexB = (vertexData & 0x00F0) >> 4;

              // Normalized vertex positions
              Vec3 vertA = regularCellVertexOffsets[indexA];
              Vec3 vertB = regularCellVertexOffsets[indexB];

              // Cell vertex indices and z-positions
              int iA = indexA & bit(0) ? i + 1 : i;
              int iB = indexB & bit(0) ? i + 1 : i;
              int jA = indexA & bit(1) ? j + 1 : j;
              int jB = indexB & bit(1) ? j + 1 : j;
              length_t zA = indexA & bit(2) ? cellAnchorZPos + cellLength : cellAnchorZPos;
              length_t zB = indexB & bit(2) ? cellAnchorZPos + cellLength : cellAnchorZPos;

              // Isovalues of vertices A and B
              length_t tA = noiseValues[iA][jA].w - zA;
              length_t tB = noiseValues[iB][jB].w - zB;

              length_t t = tA / (tA - tB);

              // Calculate the position of the vertex on the cell edge.  The smoothness parameter S is used to interpolate between 
              // roughest iso-surface (vertex is always chosen at edge midpoint) and the smooth iso-surface interpolation used by Paul Bourke.
              Vec3 normalizedCellPosition = vertA + (0.5 * (1 - S) + t * S) * (vertB - vertA);
              vertexPositions[vert] = cellLength * Vec3(i - 1, j - 1, k - 1) + cellLength * normalizedCellPosition;

              // Estimate isosurface normal using linear interpolation
              const Vec3& n0 = noiseValues[iA][jA];
              const Vec3& n1 = noiseValues[iB][jB];
              Vec3 n = t * n0 + (1 - t) * n1;
              isoNormals[vert] = glm::normalize(n);
            }

            // Calculate vertex normal
            Vec3 surfaceNormal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));
            float lightValue = static_cast<float>((1.0 + surfaceNormal.z) / 2);

            for (int vert = 0; vert < 3; ++vert)
              LODMeshData.push_back({ vertexPositions[vert], surfaceNormal, isoNormals[vert], vert, lightValue });
          }
        }
    node->data->meshData = LODMeshData;

    // Generate transition meshes
    for (BlockFace face : BlockFaceIterator())
    {
      std::vector<LOD::Vertex> transitionMeshData{};
      const int faceID = static_cast<int>(face);

      // Relabel coordinates
      const int u = faceID / 2;
      const int v = (u + 1) % 3;
      const int w = (u + 2) % 3;
      const bool facingPositiveDir = faceID % 2 == 0;

      for (int i = 1; i < numCells + 1; i += 2)
        for (int j = 1; j < numCells + 1; j += 2)
        {
          // Determine which of the 512 cases the cell belongs to
          uint16_t cellCase = 0;
          for (int p = 0; p < 9; ++p)
          {
            static constexpr int indexMapping[9] = { 0, 1, 2, 7, 8, 3, 6, 5, 4 };

            BlockIndex cellVertexIndex{};
            cellVertexIndex[u] = facingPositiveDir ? numCells + 1 : 1;
            cellVertexIndex[v] = i + p % 3;
            cellVertexIndex[w] = j + p / 3;

            const int& I = cellVertexIndex.i;
            const int& J = cellVertexIndex.j;
            const int& K = cellVertexIndex.k;
            length_t Z = node->anchor.k * Chunk::Length() + (K - 1) * cellLength;

            if (noiseValues[I][J].w > Z)
              cellCase |= bit(indexMapping[p]);
          }
          if (cellCase == 0 || cellCase == 511)
            continue;

          // Use lookup table to determine which of 56 equivalence classes the cell belongs to
          uint8_t cellEquivClass = transitionCellClass[cellCase];
          bool reverseWindingOrder = cellEquivClass >> 7;
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
            // Lookup indices of vertices A,B of the cell edge that vertex v lies on
              int edgeIndex = cellData.vertexIndex[3 * tri + vert];
              uint16_t vertexData = transitionVertexData[cellCase][edgeIndex];
              uint8_t indexA = vertexData & 0x000F;
              uint8_t indexB = (vertexData & 0x00F0) >> 4;

              // Normalized vertex positions
              Vec3 vertA = transitionCellVertexOffsets[faceID][indexA];
              Vec3 vertB = transitionCellVertexOffsets[faceID][indexB];

              static constexpr int indexMapping[13] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 2, 6, 8 };

              BlockIndex vA{};
              vA[u] = facingPositiveDir ? numCells + 1 : 1;
              vA[v] = i + indexMapping[indexA] % 3;
              vA[w] = j + indexMapping[indexA] / 3;

              BlockIndex vB{};
              vB[u] = facingPositiveDir ? numCells + 1 : 1;
              vB[v] = i + indexMapping[indexB] % 3;
              vB[w] = j + indexMapping[indexB] / 3;

              // Cell vertex indices and z-positions
              const int& iA = vA.i;
              const int& iB = vB.i;
              const int& jA = vA.j;
              const int& jB = vB.j;
              const int& kA = vA.k;
              const int& kB = vB.k;
              length_t zA = node->anchor.k * Chunk::Length() + (kA - 1) * cellLength;
              length_t zB = node->anchor.k * Chunk::Length() + (kB - 1) * cellLength;

              // Isovalues of vertices A and B
              length_t tA = noiseValues[iA][jA].w - zA;
              length_t tB = noiseValues[iB][jB].w - zB;

              length_t t = tA / (tA - tB);

              // Calculate the position of the vertex on the cell edge.  The smoothness parameter S is used to interpolate between 
              // roughest iso-surface (vertex is always chosen at edge midpoint) and the smooth iso-surface interpolation used by Paul Bourke.
              Vec3 normalizedCellPosition = vertA + (0.5 * (1 - S) + t * S) * (vertB - vertA);

              // Squash transition cell towards LOD face
              length_t adjustment = static_cast<length_t>(facingPositiveDir ? 0.25 : 0.0);
              Vec3 transitionCellPosition = normalizedCellPosition;
              transitionCellPosition[u] = static_cast<length_t>(0.25) * transitionCellPosition[u] + adjustment;  // NOTE: Make more general

              Vec3 localCellAnchor{};
              localCellAnchor[u] = (facingPositiveDir ? numCells - 1 : 0) * cellLength;
              localCellAnchor[v] = (i - 1) * cellLength;
              localCellAnchor[w] = (j - 1) * cellLength;

              // NOTE: Currently bug where transition cells facing negative direction have incorrect winding order, investigate.
              int vertexOrder = reverseWindingOrder ? 2 - vert : vert;
              vertexPositions[vertexOrder] = localCellAnchor + 2 * cellLength * transitionCellPosition;

              // Estimate isosurface normal using linear interpolation
              const Vec3& n0 = noiseValues[iA][jA];
              const Vec3& n1 = noiseValues[iB][jB];
              Vec3 n = t * n0 + (1 - t) * n1;
              isoNormals[vertexOrder] = glm::normalize(n);
            }

            // Calculate vertex normal
            Vec3 surfaceNormal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));

            float lightValue = static_cast<float>((1.0 + surfaceNormal.z) / 2);

            for (int vert = 0; vert < 3; ++vert)
              transitionMeshData.push_back({ vertexPositions[vert], surfaceNormal, isoNormals[vert], vert, lightValue });
          }
        }

      node->data->transitionMeshData[faceID] = transitionMeshData;
    }
  }

  void UpdateMesh(LOD::Octree& tree, LOD::Octree::Node* node)
  {
    std::vector<LOD::Vertex> LODMesh = CalcAdjustedPrimaryMesh(tree, node);

  // Generate vertex array
    Engine::Shared<Engine::VertexArray> LODVertexArray = Engine::VertexArray::Create();
    auto LODVertexBuffer = Engine::VertexBuffer::Create(static_cast<uint32_t>(LODMesh.size()) * sizeof(LOD::Vertex));
    LODVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                 { ShaderDataType::Float3, "a_SurfaceNormal"},
                                 { ShaderDataType::Float3, "a_IsoNormal"},
                                 { ShaderDataType::Int,    "a_QuadIndex"},
                                 { ShaderDataType::Float,  "s_LightValue"} });
    LODVertexArray->addVertexBuffer(LODVertexBuffer);

    uintptr_t dataSize = sizeof(LOD::Vertex) * LODMesh.size();
    LODVertexBuffer->setData(LODMesh.data(), dataSize);
    node->data->vertexArray = LODVertexArray;

    for (BlockFace face : BlockFaceIterator())
    {
      const int faceID = static_cast<int>(face);
      std::vector<LOD::Vertex> transitionMesh = CalcAdjustedTransitionMesh(tree, node, face);

      // Generate vertex array
      Engine::Shared<Engine::VertexArray> transitionVertexArray = Engine::VertexArray::Create();
      auto transitionVertexBuffer = Engine::VertexBuffer::Create(static_cast<uint32_t>(transitionMesh.size()) * sizeof(LOD::Vertex));
      transitionVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                          { ShaderDataType::Float3, "a_SurfaceNormal"},
                                          { ShaderDataType::Float3, "a_IsoNormal"},
                                          { ShaderDataType::Int,    "a_QuadIndex"},
                                          { ShaderDataType::Float,  "s_LightValue"} });
      transitionVertexArray->addVertexBuffer(transitionVertexBuffer);

      uintptr_t dataSize = sizeof(LOD::Vertex) * transitionMesh.size();
      transitionVertexBuffer->setData(transitionMesh.data(), dataSize);
      node->data->transitionVertexArrays[faceID] = transitionVertexArray;
    }
  }

  std::vector<LOD::Vertex> CalcAdjustedPrimaryMesh(LOD::Octree& tree, LOD::Octree::Node* node)
  {
    // Number of cells in each direction
    constexpr int numCells = Chunk::Size();

    const length_t cellLength = node->length() / numCells;

    const GlobalIndex offsets[6] = { { node->size(), 0, 0}, { -1, 0, 0}, { 0, node->size(), 0}, { 0, -1, 0}, { 0, 0, node->size()}, { 0, 0, -1} };
                                //            East              West             North             South             Top               Bottom

    // Determine which faces transition to a lower resolution LOD
    std::array<bool, 6> isTransitionFace{}; // NOTE: Can be changed to a single byte
    for (BlockFace face : BlockFaceIterator())
    {
      const int faceID = static_cast<int>(face);

      LOD::Octree::Node* neighbor = tree.findLeaf(node->anchor + offsets[faceID]);
      if (neighbor == nullptr)
        continue;

      if (node->LODLevel() == neighbor->LODLevel())
        continue;
      else if (neighbor->LODLevel() - node->LODLevel() == 1)
        isTransitionFace[faceID] = true;
      else if (neighbor->LODLevel() - node->LODLevel() > 1)
        EN_WARN("LOD neighbor is more than one level higher resolution");
    }
    node->data->isTransitionFace = isTransitionFace;

    std::vector<LOD::Vertex> LODMesh = node->data->meshData;

    if (isTransitionFace[static_cast<int>(BlockFace::East)] || isTransitionFace[static_cast<int>(BlockFace::West)] ||
      isTransitionFace[static_cast<int>(BlockFace::North)] || isTransitionFace[static_cast<int>(BlockFace::South)] ||
      isTransitionFace[static_cast<int>(BlockFace::Top)] || isTransitionFace[static_cast<int>(BlockFace::Bottom)])
    {
      // Formulas can be found in section 4.4 of TransVoxel paper
      const length_t transitionCellWidth = cellLength / 2;
      auto isNearFace = [=](bool facingPositiveDir, length_t u)
      {
        return facingPositiveDir ? u > cellLength * (numCells - 1) : u < cellLength;
      };
      auto calcAdjustment = [=](bool facingPositiveDir, length_t u)
      {
        return facingPositiveDir ? (numCells - 1 - u / cellLength) * transitionCellWidth : (1 - u / cellLength) * transitionCellWidth;
      };

      // Adjust coorindates of boundary cells on primary LOD mesh
      for (int i = 0; i < LODMesh.size(); ++i)
      {
        Vec3 adjustment{};
        bool isNearSameResolutionLOD = false;
        for (BlockFace face : BlockFaceIterator())
        {
          const int faceID = static_cast<int>(face);
          const int coordID = faceID / 2;
          const bool facingPositiveDir = faceID % 2 == 0;

          if (isNearFace(facingPositiveDir, LODMesh[i].position[coordID]))
          {
            if (isTransitionFace[faceID])
              adjustment[coordID] = calcAdjustment(facingPositiveDir, LODMesh[i].position[coordID]);
            else
            {
              isNearSameResolutionLOD = true;
              break;
            }
          }
        }

        if (!isNearSameResolutionLOD && adjustment != Vec3(0.0))
        {
          const Vec3& n = LODMesh[i].isoNormal;
          const Mat3 transform = Mat3(  1 - n.x * n.x,  -n.x * n.y,      -n.x * n.z,
                                       -n.x * n.y,       1 - n.y * n.y,  -n.y * n.z,
                                       -n.x * n.z,      -n.y * n.z,       1 - n.z * n.z);

          LODMesh[i].position += transform * adjustment;
        }
      }
    }
    return LODMesh;
  }

  std::vector<LOD::Vertex> CalcAdjustedTransitionMesh(LOD::Octree& tree, LOD::Octree::Node* node, BlockFace transitionMesh)
  {
    // Number of cells in each direction
    constexpr int numCells = Chunk::Size();

    const int meshID = static_cast<int>(transitionMesh);
    const length_t cellLength = node->length() / numCells;

    const GlobalIndex offsets[6] = { { node->size(), 0, 0}, { -1, 0, 0}, { 0, node->size(), 0}, { 0, -1, 0}, { 0, 0, node->size()}, { 0, 0, -1} };
                                //            East              West             North             South             Top               Bottom

    // Determine which faces transition to a lower resolution LOD
    std::array<bool, 6> isTransitionFace{}; // NOTE: Can be changed to a single byte
    for (BlockFace face : BlockFaceIterator())
    {
      const int faceID = static_cast<int>(face);

      LOD::Octree::Node* neighbor = tree.findLeaf(node->anchor + offsets[faceID]);
      if (neighbor == nullptr)
        continue;

      if (node->LODLevel() == neighbor->LODLevel())
        continue;
      else if (neighbor->LODLevel() - node->LODLevel() == 1)
        isTransitionFace[faceID] = true;
      else if (neighbor->LODLevel() - node->LODLevel() > 1)
        EN_WARN("LOD neighbor is more than one level higher resolution");
    }
    node->data->isTransitionFace = isTransitionFace;

    std::vector<LOD::Vertex> LODMesh = node->data->transitionMeshData[meshID];

    if (isTransitionFace[static_cast<int>(BlockFace::East)] || isTransitionFace[static_cast<int>(BlockFace::West)] ||
      isTransitionFace[static_cast<int>(BlockFace::North)] || isTransitionFace[static_cast<int>(BlockFace::South)] ||
      isTransitionFace[static_cast<int>(BlockFace::Top)] || isTransitionFace[static_cast<int>(BlockFace::Bottom)])
    {
      // Formulas can be found in section 4.4 of TransVoxel paper
      const length_t transitionCellWidth = cellLength / 2;
      auto isNearFace = [=](bool facingPositiveDir, length_t u)
      {
        return facingPositiveDir ? u > cellLength * (numCells - 1) : u < cellLength;
      };
      auto calcAdjustment = [=](bool facingPositiveDir, length_t u)
      {
        return facingPositiveDir ? (numCells - 1 - u / cellLength) * transitionCellWidth : (1 - u / cellLength) * transitionCellWidth;
      };

      // Adjust coorindates of boundary cells on transition mesh
      int numTriangles = static_cast<int>(LODMesh.size()) / 3;
      for (int tri = 0; tri < numTriangles; ++tri)
      {
        bool triangleModified = false;
        for (int vert = 0; vert < 3; ++vert)
        {
          int i = 3 * tri + vert;

          // If Vertex is on low-resolution side, skip
          if (LODMesh[i].position[meshID / 2] < 0.0001 * node->length() || LODMesh[i].position[meshID / 2] > 0.9999 * node->length())
            continue;
          else
            LODMesh[i].position[meshID / 2] = (meshID % 2 == 0) ? node->length() : static_cast<length_t>(0.0);

          Vec3 adjustment{};
          bool isNearSameResolutionLOD = false;
          for (BlockFace face : BlockFaceIterator())
          {
            const int faceID = static_cast<int>(face);
            const int coordID = faceID / 2;
            const bool facingPositiveDir = faceID % 2 == 0;

            if (isNearFace(facingPositiveDir, LODMesh[i].position[coordID]))
            {
              if (isTransitionFace[faceID])
                adjustment[coordID] = calcAdjustment(facingPositiveDir, LODMesh[i].position[coordID]);
              else
              {
                isNearSameResolutionLOD = true;
                break;
              }
            }
          }

          if (!isNearSameResolutionLOD && adjustment != Vec3(0.0))
          {
            const Vec3& n = LODMesh[i].isoNormal;
            const Mat3 transform = Mat3(  1 - n.x * n.x,  -n.x * n.y,      -n.x * n.z,
                                         -n.x * n.y,       1 - n.y * n.y,  -n.y * n.z,
                                         -n.x * n.z,      -n.y * n.z,       1 - n.z * n.z);

            LODMesh[i].position += transform * adjustment;

            triangleModified = true;
          }
        }

        // Recalculate surface normals
        if (triangleModified)
        {
          std::array<Vec3, 3> vertexPositions = { LODMesh[3 * tri].position, LODMesh[3 * tri + 1].position , LODMesh[3 * tri + 2].position };

          Vec3 surfaceNormal = glm::normalize(glm::cross(vertexPositions[1] - vertexPositions[0], vertexPositions[2] - vertexPositions[0]));
          float lightValue = static_cast<float>((1.0 + surfaceNormal.z) / 2);

          for (int vert = 0; vert < 3; ++vert)
          {
            LODMesh[3 * tri + vert].surfaceNormal = surfaceNormal;
            LODMesh[3 * tri + vert].lightValue = lightValue;
          }
        }
      }
    }
    return LODMesh;
  }
}
