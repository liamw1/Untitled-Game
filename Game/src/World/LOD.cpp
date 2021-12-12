#include "GMpch.h"
#include "LOD.h"

Octree::Octree()
  : m_Root(Node(s_RootNodeAnchor, 0))
{
  m_Root.leaf = new LOD(s_RootNodeAnchor, s_MaxNodeDepth);
}

void Octree::splitNode(const LOD& leaf)
{
  Node& node = findNode(leaf);
  const int64_t nodeChildSize = bit(s_MaxNodeDepth - node.depth) / 2;

  if (node.depth == s_MaxNodeDepth)
  {
    EN_ERROR("Node is already at max depth");
    return;
  }

  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
      {
        int childIndex = 4 * i + 2 * j + k;
        EN_ASSERT(node.children[childIndex] == nullptr, "Child node already exists!");

        GlobalIndex nodeChildAnchor = node.anchor + nodeChildSize * GlobalIndex({ i, j, k });
        node.children[childIndex] = new Node(nodeChildAnchor, node.depth + 1);
        node.children[childIndex]->leaf = new LOD(nodeChildAnchor, node.leaf->level - 1);
      }

  delete node.leaf;
  node.leaf = nullptr;
}

std::vector<LOD*> Octree::getLeaves() const
{
  std::vector<LOD*> leaves{};
  getLeaf(m_Root, leaves);

  return leaves;
}



Octree::Node& Octree::findNode(const LOD& leaf)
{
  return findNode(m_Root, leaf);
}

Octree::Node& Octree::findNode(Node& node, const LOD& leaf)
{
  const int leafDepth = s_MaxNodeDepth - leaf.level;
  const int64_t nodeChildSize = bit(s_MaxNodeDepth - node.depth) / 2;

  if (node.leaf != nullptr)
  {
    EN_ASSERT(&leaf == node.leaf, "Node could not be found!");
    return node;
  }
  else if (node.depth < leafDepth)
  {
    GlobalIndex nodeCenter = node.anchor + nodeChildSize * GlobalIndex({ 1, 1, 1 });
    int childIndex = 4 * (leaf.anchor.i >= nodeCenter.i) + 2 * (leaf.anchor.j >= nodeCenter.j) + (leaf.anchor.k >= nodeCenter.k);

    if (node.children[childIndex] != nullptr)
      return findNode(*node.children[childIndex], leaf);
    else
      EN_ERROR("Node could not be found!");
  }

  EN_ERROR("Node could not be found!");
  return m_Root;
}

void Octree::getLeaf(const Node& node, std::vector<LOD*>& leaves) const
{
  if (node.leaf != nullptr)
  {
    leaves.push_back(node.leaf);
    return;
  }
  else if (node.depth < s_MaxNodeDepth)
    for (int i = 0; i < 8; ++i)
      if (node.children[i] != nullptr)
        getLeaf(*node.children[i], leaves);
}
