#pragma once
#include "Chunk.h"

struct LOD
{
  const GlobalIndex anchor;
  const uint8_t level;

  LOD(const GlobalIndex& anchorIndex, uint8_t LODLevel)
    : anchor(anchorIndex), level(LODLevel) {}
};

class Octree
{
public:
  Octree();

  void splitNode(const LOD& leaf);

  std::vector<LOD*> getLeaves() const;

private:
  /*
    A representation of an octree node.
    The node has children if and only if leaf is nullptr.
  */
  struct Node
  {
    const GlobalIndex anchor;
    const uint8_t depth;

    LOD* leaf = nullptr;
    std::array<Node*, 8> children{};

    Node(const GlobalIndex& anchorIndex, uint8_t nodeDepth)
      : anchor(anchorIndex), depth(nodeDepth) {}

    ~Node()
    {
      delete leaf;
      leaf = nullptr;
      for (int i = 0; i < 8; ++i)
      {
        delete children[i];
        children[i] = nullptr;
      }
    }
  };

  static constexpr uint8_t s_MaxNodeDepth = 10;
  static constexpr uint64_t s_RootNodeSize = bit(s_MaxNodeDepth);
  static constexpr GlobalIndex s_RootNodeAnchor = -static_cast<int64_t>(s_RootNodeSize / 2) * GlobalIndex({ 1, 1, 1 });

  Node m_Root;

  Node& findNode(const LOD& leaf);
  Node& findNode(Node& node, const LOD& leaf);

  void getLeaf(const Node& node, std::vector<LOD*>& leaves) const;
};