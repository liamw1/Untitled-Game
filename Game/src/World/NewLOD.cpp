#include "GMpch.h"
#include "NewLOD.h"

namespace newLod
{
  Node::Node()
    : Node({}, -1) {}
  Node::Node(const GlobalIndex& anchor, globalIndex_t depth)
    : m_Anchor(anchor), m_Depth(depth)
  {
    m_Data = std::make_unique<Data>();
  }

  bool Node::isRoot() const { return m_Depth == 0; }
  bool Node::isLeaf() const { return !m_Children; }
  i32 Node::lodLevel() const { return c_MaxNodeDepth - m_Depth; }

  globalIndex_t Node::size() const { return eng::math::pow2<globalIndex_t>(lodLevel()); }

  void Node::split()
  {
    ENG_ASSERT(m_Depth < MaxDepth(), "Node is already at max depth!");
    if (!isLeaf())
      return;

    m_Children = std::make_unique<Node[]>(c_NumChildren);

    globalIndex_t childSize = size() / 2;
    GlobalBox childBox(0, 1);
    childBox.forEach([this, childSize, &childBox](const GlobalIndex& child)
    {
      uSize childIndex = childBox.linearIndexOf(child);
      GlobalIndex childAnchor = m_Anchor + childSize * child;
      m_Children[childIndex] = Node(childAnchor, m_Depth + 1);
    });
  }

  void Node::combine()
  {
    if (isLeaf())
      return;

    std::for_each_n(m_Children.get(), c_NumChildren, [](Node& child) { child.combine(); });
    m_Children.reset();
  }



  Octree::Octree()
    : m_Root(c_RootNodeAnchor, 0) {}
}
