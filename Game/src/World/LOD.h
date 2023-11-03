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
namespace lod
{
  struct Vertex
  {
    eng::math::Float3 position;
    eng::math::Float3 isoNormal;
    std::array<i32, 2> textureIndices;
    eng::math::Float2 textureWeights;
    i32 quadIndex;

    Vertex(const eng::math::Float3& position, const eng::math::Float3& isoNormal, const std::array<i32, 2>& textureIndices, const eng::math::Float2& textureWeights, i32 quadIndex);
  };

  struct UniformData
  {
    eng::math::Float3 anchor;
    f32 textureScaling;
    const f32 nearPlaneDistance = 10 * block::lengthF();
    const f32 farPlaneDistance = 1e10f * block::lengthF();
  };

  struct MeshData
  {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    std::unique_ptr<eng::VertexArray> vertexArray;

    MeshData();

    static void BindBuffers();
    static void SetUniforms(const UniformData& uniformData);

  private:
    static constexpr i32 c_TextureSlot = 0;
    static constexpr i32 c_UniformBinding = 3;
    static inline std::once_flag s_InitializedFlag;
    static inline std::unique_ptr<eng::Shader> s_Shader = nullptr;
    static inline std::unique_ptr<eng::Uniform> s_Uniform = nullptr;
    static inline std::shared_ptr<eng::TextureArray> s_TextureArray = nullptr;
    static inline const eng::BufferLayout s_VertexBufferLayout = { { eng::ShaderDataType::Float3, "a_Position"        },
                                                                   { eng::ShaderDataType::Float3, "a_IsoNormal"       },
                                                                   { eng::ShaderDataType::Int2,   "a_TextureIndices"  },
                                                                   { eng::ShaderDataType::Float2, "a_TextureWeighs"   },
                                                                   { eng::ShaderDataType::Int,    "a_QuadIndex"       } };

    static void Initialize();
  };

  struct Data 
  {
    MeshData primaryMesh;
    eng::EnumArray<MeshData, eng::math::Direction> transitionMeshes;

    u8 transitionFaces = 0;
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
      std::array<Node*, 8> children;
      const i32 depth;

      const GlobalIndex anchor;
      Data* data = nullptr;

      Node(Node* parentNode, i32 nodeDepth, const GlobalIndex& anchorIndex);
      ~Node();

      bool isRoot() const;
      bool isLeaf() const;
      i32 LODLevel() const;

      /*
        \returns Size of LOD in each direction, given in units of chunks.
      */
      globalIndex_t size() const;

      /*
        \returns The length of LOD in each direction, given in physical units.
      */
      length_t length() const;

      /*
        An LOD's anchor point is its bottom southeast vertex.
        Position given relative to the anchor of the origin chunk.

        Useful property:
        If the anchor point is denoted by A, then for any point
        X within the LOD, X_i >= A_i.
      */
      eng::math::Vec3 anchorPosition() const;

      /*
        \returns The LOD's geometric center relative to origin chunk.
      */
      eng::math::Vec3 center() const;

      /*
        \returns The LOD's axis-aligned bounding box (AABB), 
        given in units of chunks.
      */
      GlobalBox boundingBox() const;
    };

  private:
    static constexpr i32 c_MaxNodeDepth = 12;
    static constexpr u64 c_RootNodeSize = eng::pow2(c_MaxNodeDepth);
    static constexpr GlobalIndex c_RootNodeAnchor = -eng::arithmeticCast<globalIndex_t>(c_RootNodeSize / 2) * GlobalIndex(1, 1, 1);

    // Root node of the tree
    Node m_Root;

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

    static constexpr i32 MaxNodeDepth() { return c_MaxNodeDepth; }

  private:
    /*
      Helper functions for node searching.
    */
    void getLeavesPriv(Node* branch, std::vector<Node*>& leaves);
    Node* findLeafPriv(Node* branch, const GlobalIndex& index);
  };

  void draw(const Octree::Node* leaf);

  /*
    Generates primary and transition meshes for the given node
    using terrain noise.  LOD needs to be updated before these
    meshes are uploaded to the GPU.
  */
  void generateMesh(Octree::Node* node);

  /*
    Adjusts meshes based on surrounding LODs and uploads
    mesh data to the GPU for rendering.

    NOTE: Needs to be made faster.
  */
  void updateMesh(Octree& tree, Octree::Node* node);

  void messageNeighbors(Octree& tree, Octree::Node* node);
}