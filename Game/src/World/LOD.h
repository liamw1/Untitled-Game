#pragma once
#include "Indexing.h"
#include "Chunk.h"

/*
  Level of Detail (LOD) system.  The game world is partitioned with an octree,
  the leaf nodes of which we often refer to as an "LOD".  Each LOD is given an
  "LOD level" based on its depth in the tree.  This LOD level represents the
  level of simplification of the voxel/noise data.  At level 0, LODs have as
  many polygons as chunks, while at high LOD levels individual polygons can
  span multiple chunks.
*/
namespace LOD
{
  struct AABB
  {
    GlobalIndex min;
    GlobalIndex max;
  };

  struct Vertex
  {
    Vertex() = delete;
    Vertex(const Float3& position, const Float3& isoNormal, const std::array<int, 2>& textureIndices, const Float2& textureWeights, int quadIndex)
      : position(position), isoNormal(isoNormal), textureIndices(textureIndices), textureWeights(textureWeights), quadIndex(quadIndex) {}

    Float3 position;
    Float3 isoNormal;
    std::array<int, 2> textureIndices;
    Float2 textureWeights;
    int quadIndex;
  };

  struct Uniforms
  {
    Float3 anchor;
    float textureScaling;
    const float nearPlaneDistance = static_cast<float>(10 * Block::Length());
    const float farPlaneDistance = static_cast<float>(1e10 * Block::Length());
  };

  struct MeshData
  {
    MeshData()
      : vertices(), indices()
    {
      vertexArray = Engine::VertexArray::Create();
      vertexArray->setLayout(s_VertexBufferLayout);
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Unique<Engine::VertexArray> vertexArray;

    static void Initialize(const Shared<Engine::TextureArray>& textureArray);
    static void BindBuffers();
    static void SetUniforms(const Uniforms& uniforms);

  private:
    static constexpr int s_TextureSlot = 0;
    static constexpr int s_UniformBinding = 3;
    static inline Unique<Engine::Shader> s_Shader = nullptr;
    static inline Shared<Engine::TextureArray> s_TextureArray = nullptr;
    static inline const Engine::BufferLayout s_VertexBufferLayout = { { ShaderDataType::Float3, "a_Position"        },
                                                                      { ShaderDataType::Float3, "a_IsoNormal"       },
                                                                      { ShaderDataType::Int2,   "a_TextureIndices"  },
                                                                      { ShaderDataType::Float2, "a_TextureWeighs"   },
                                                                      { ShaderDataType::Int,    "a_QuadIndex"       } };
  };

  struct Data 
  {
    MeshData primaryMesh{};
    std::array<MeshData, 6> transitionMeshes{};

    uint8_t transitionFaces = 0;
    bool meshGenerated = false;
    bool needsUpdate = true;
  };

  class Octree
  {
  public:
    /*
      A representation of an octree-based LOD node.
      The node has children if and only if data is nullptr.
      The node is the root node if and only if parent is nullptr.
    */
    struct Node
    {
      Node* const parent;
      std::array<Node*, 8> children{};
      const int depth;

      const GlobalIndex anchor;
      Data* data = nullptr;

      Node(Node* parentNode, int nodeDepth, const GlobalIndex& anchorIndex)
        : parent(parentNode), depth(nodeDepth), anchor(anchorIndex) {}

      ~Node()
      {
        delete data;
        data = nullptr;
        for (int i = 0; i < 8; ++i)
        {
          delete children[i];
          children[i] = nullptr;
        }
      }

      bool isRoot() const { return parent == nullptr; }
      bool isLeaf() const { return data != nullptr; }

      int LODLevel() const { return s_MaxNodeDepth - depth; }

      /*
        \returns Size of LOD in each direction, given in units of chunks.
      */
      globalIndex_t size() const { return static_cast<globalIndex_t>(pow2(LODLevel())); }

      /*
        \returns The length of LOD in each direction, given in physical units.
      */
      length_t length() const { return size() * Chunk::Length(); }

      /*
        An LOD's anchor point is its bottom southeast vertex.
        Position given relative to the anchor of the origin chunk.

        Useful property:
        If the anchor point is denoted by A, then for any point
        X within the LOD, X_i >= A_i.
      */
      Vec3 anchorPosition() const;

      /*
        \returns The LOD's geometric center relative to origin chunk.
      */
      Vec3 center() const { return anchorPosition() + length() / 2; }

      /*
        \returns The LOD's axis-aligned bounding box (AABB), 
        given in units of chunks.
      */
      AABB boundingBox() const { return { anchor, anchor + size() }; }
    };

  public:
    Octree();

    /*
      Divides given node into 8 equally-sized child nodes.
      Deletes data within the given node.
    */
    void splitNode(Node* node);

    /*
      Merges the 8 children of the given node into a single node.
      Deletes data within the child nodes.
    */
    void combineChildren(Node* node);

    /*
      \returns A list of all leaf nodes within the octree.
               NOTE: Could probably be made faster with caching.
    */
    std::vector<Node*> getLeaves();

    /*
      \returns The leaf node that contains the given index.
               Will return nullptr if index is outside of octree bounds.
    */
    Node* findLeaf(const GlobalIndex& index);

    static constexpr int MaxNodeDepth() { return s_MaxNodeDepth; }

  private:
    static constexpr int s_MaxNodeDepth = 12;
    static constexpr uint64_t s_RootNodeSize = pow2(s_MaxNodeDepth);
    static constexpr GlobalIndex s_RootNodeAnchor = -static_cast<globalIndex_t>(s_RootNodeSize / 2) * GlobalIndex(1, 1, 1);

    // Root node of the tree
    Node m_Root;

    /*
      Helper functions for node searching.
    */
    void getLeavesPriv(Node* branch, std::vector<Node*>& leaves);
    Node* findLeafPriv(Node* branch, const GlobalIndex& index);
  };

  void Draw(const Octree::Node* leaf);

  /*
    \returns True if the given AABBs intersect.  Two AABBs sharing
             a border does not count as an intersection.
  */
  bool Intersection(AABB boxA, AABB boxB);

  /*
    Generates primary and transition meshes for the given node
    using terrain noise.  LOD needs to be updated before these
    meshes are uploaded to the GPU.
  */
  void GenerateMesh(LOD::Octree::Node* node);

  /*
    Adjusts meshes based on surrounding LODs and uploads
    mesh data to the GPU for rendering.

    NOTE: Needs to be made faster.
  */
  void UpdateMesh(LOD::Octree& tree, LOD::Octree::Node* node);

  void MessageNeighbors(LOD::Octree& tree, LOD::Octree::Node* node);
}