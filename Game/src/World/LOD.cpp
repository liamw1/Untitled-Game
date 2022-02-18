#include "GMpch.h"
#include "LOD.h"
#include "Player/Player.h"

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
}
