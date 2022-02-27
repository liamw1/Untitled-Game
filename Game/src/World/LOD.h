#pragma once
#include "Indexing.h"
#include "Chunk.h"

namespace LOD
{
  struct AABB
  {
    GlobalIndex min;
    GlobalIndex max;
  };

  struct Vertex
  {
    Float3 position;
    Float3 isoNormal;
    int quadIndex;
    float lightValue;
  };

  struct Data 
  {
    std::vector<Vertex> meshData;
    Engine::Shared<Engine::VertexArray> vertexArray;

    std::array<std::vector<Vertex>, 6> transitionMeshData;
    std::array<Engine::Shared<Engine::VertexArray>, 6> transitionVertexArrays;

    uint8_t transitionFaces = 0;
    bool meshGenerated = false;
    bool needsUpdate = false;
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
    public:
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
      int64_t size() const { return pow2(LODLevel()); }
      length_t length() const { return size() * Chunk::Length(); }

      Vec3 anchorPosition() const;
      Vec3 center() const { return anchorPosition() + length() / 2; }

      AABB boundingBox() const { return { anchor, anchor + size() }; }
    };

  public:
    Octree();

    void splitNode(Node* node);
    void combineChildren(Node* node);

    std::vector<Node*> getLeaves();
    Node* findLeaf(const GlobalIndex& index);

    static constexpr int MaxNodeDepth() { return s_MaxNodeDepth; }

  private:
    static constexpr int s_MaxNodeDepth = 8;
    static constexpr uint64_t s_RootNodeSize = pow2(s_MaxNodeDepth);
    static constexpr GlobalIndex s_RootNodeAnchor = -static_cast<globalIndex_t>(s_RootNodeSize / 2) * GlobalIndex({ 1, 1, 1 });

    // Root node of the tree
    Node m_Root;

    void getLeavesPriv(Node* branch, std::vector<Node*>& leaves);
    Node* findLeafPriv(Node* branch, const GlobalIndex& index);
  };

  bool Intersection(AABB boxA, AABB boxB);

  void DetermineTransitionFaces(LOD::Octree& tree, LOD::Octree::Node* node);

  void GenerateMesh(LOD::Octree::Node* node);
  void UpdateMesh(LOD::Octree& tree, LOD::Octree::Node* node);
}