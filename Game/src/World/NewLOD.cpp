#include "GMpch.h"
#include "NewLOD.h"

namespace newLod
{
  Node::Node()
    : Node({}, -1) {}
  Node::Node(const GlobalIndex& anchor, globalIndex_t depth)
    : m_Anchor(anchor), m_Depth(depth), m_Children(c_ChildIndexBounds, eng::AllocationPolicy::Deferred)
  {
    m_Data = std::make_shared<Data>();
  }

  bool Node::isRoot() const { return m_Depth == 0; }
  bool Node::isLeaf() const { return !m_Children; }
  i32 Node::lodLevel() const { return c_MaxNodeDepth - m_Depth; }

  globalIndex_t Node::size() const { return eng::math::pow2<globalIndex_t>(lodLevel()); }

  const BlockArrayBox<Node>& Node::children() const { return m_Children; }
  std::shared_ptr<Data> Node::data() { return m_Data; }

  void Node::split()
  {
    ENG_ASSERT(m_Depth < MaxDepth(), "Node is already at max depth!");
    if (!isLeaf())
      return;

    m_Children.allocate();

    globalIndex_t childSize = size() / 2;
    c_ChildIndexBounds.forEach([this, childSize](const BlockIndex& childIndex)
    {
      GlobalIndex childAnchor = m_Anchor + childSize * childIndex.upcast<globalIndex_t>();
      m_Children(childIndex) = Node(childAnchor, m_Depth + 1);
    });
  }

  void Node::combine()
  {
    if (isLeaf())
      return;

    m_Children.forEach([](Node& child) { child.combine(); });
    m_Children.clear();
  }



  Octree::Octree()
    : m_Root(c_RootNodeAnchor, 0) {}

  std::shared_ptr<Data> Octree::find(const GlobalIndex& index)
  {
    if (c_Bounds.encloses(index))
      return nullptr;
    return findImpl(m_Root, index);
  }

  std::shared_ptr<Data> Octree::findImpl(Node& branch, const GlobalIndex& index)
  {
    if (branch.isLeaf())
      return branch.data();

    return nullptr;
  }




  DrawCommand::DrawCommand(const GlobalIndex& anchor)
    : eng::IndexedDrawCommand<DrawCommand, GlobalIndex>(anchor) {}

  u32 DrawCommand::vertexCount() const { return u32(); }
  eng::mem::IndexData DrawCommand::indexData() const { return eng::mem::IndexData(); }
  eng::mem::Data DrawCommand::vertexData() const { return eng::mem::Data(); }
  void DrawCommand::clearData() {}
}
