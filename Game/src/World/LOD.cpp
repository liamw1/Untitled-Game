#include "GMpch.h"
#include "LOD.h"

namespace LOD
{
  Octree::Octree()
    : m_Root(Node(nullptr, 0, s_RootNodeAnchor))
  {
    m_Root.data = new Data();
  }

  void Octree::splitNode(Node* node)
  {
    EN_ASSERT(node != nullptr, "Node can't be nullptr!");
    EN_ASSERT(node->data != nullptr, "Node must be a leaf node!");
    EN_ASSERT(node->depth != s_MaxNodeDepth, "Node is already at max depth!");

    const int64_t nodeChildSize = bit(s_MaxNodeDepth - node->depth - 1);

    // Divide node into 8 equal-sized child nodes
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        for (int k = 0; k < 2; ++k)
        {
          int childIndex = 4 * i + 2 * j + k;
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

    // If data is nullptr, node is already a leaf node
    if (node->data != nullptr)
      return;

    // Delete child nodes
    for (int i = 0; i < 8; ++i)
    {
      delete node->children[i];
      node->children[i] = nullptr;
    }

    // Node becomes new leaf node
    EN_ASSERT(node->data == nullptr, "Node already has data!");
    node->data = new Data();
  }

  std::vector<Octree::Node*> Octree::getLeaves()
  {
    // Recursively collect leaf nodes
    std::vector<Node*> leaves{};
    getLeavesPriv(&m_Root, leaves);

    return leaves;
  }



  void Octree::getLeavesPriv(Node* branch, std::vector<Node*>& leaves)
  {
    if (branch->data != nullptr)
      leaves.push_back(branch);
    else if (branch->depth < s_MaxNodeDepth)
      for (int i = 0; i < 8; ++i)
        if (branch->children[i] != nullptr)
          getLeavesPriv(branch->children[i], leaves);
  }
}
