#pragma once
#include "Indexing.h"

namespace LOD
{
  struct AABB
  {
    GlobalIndex min;
    GlobalIndex max;
  };

  struct Data 
  {
    Engine::Shared<Engine::VertexArray> vertexArray;
    uint32_t meshSize;
  };

  class Octree
  {
  public:
    /*
      A representation of an octree node.
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

      int LODLevel() const { return s_MaxNodeDepth - depth; }
      int64_t size() const { return pow2(LODLevel()); }

      AABB boundingBox() const { return { anchor, anchor + size() }; }
    };

  public:
    Octree();

    void splitNode(Node* node);
    void combineChildren(Node* node);

    std::vector<Node*> getLeaves();

    static constexpr int MaxNodeDepth() { return s_MaxNodeDepth; }

  private:
    static constexpr int s_MaxNodeDepth = 10;
    static constexpr uint64_t s_RootNodeSize = pow2(s_MaxNodeDepth);
    static constexpr GlobalIndex s_RootNodeAnchor = -static_cast<globalIndex_t>(s_RootNodeSize / 2) * GlobalIndex({ 1, 1, 1 });

    // Root node of the tree
    Node m_Root;

    void getLeavesPriv(Node* branch, std::vector<Node*>& leaves);
  };

  bool Intersection(AABB boxA, AABB boxB);
}