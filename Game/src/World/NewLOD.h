#pragma once
#include "Indexing/Definitions.h"

namespace newLod
{
  struct Data
  {
  };

  class Node
  {
    static constexpr i32 c_MaxNodeDepth = 8;
    static constexpr BlockBox c_ChildIndexBounds = BlockBox(0, 1);

    GlobalIndex m_Anchor;
    globalIndex_t m_Depth;

    BlockArrayBox<Node> m_Children;
    std::shared_ptr<Data> m_Data;

  public:
    Node();
    Node(const GlobalIndex& anchor, globalIndex_t depth);

    bool isRoot() const;
    bool isLeaf() const;
    i32 lodLevel() const;

    /*
      \returns Size of LOD in each direction, given in units of chunks.
    */
    globalIndex_t size() const;

    const BlockArrayBox<const Node>& children() const;
    std::shared_ptr<Data> data();

    void split();
    void combine();

    static constexpr i32 MaxDepth() { return c_MaxNodeDepth; }
  };

  class Octree
  {
    static constexpr u64 c_RootNodeSize = eng::math::pow2<u64>(Node::MaxDepth());
    static constexpr GlobalIndex c_RootNodeAnchor = -eng::arithmeticCast<globalIndex_t>(c_RootNodeSize / 2) * GlobalIndex(1);
    static constexpr GlobalBox c_Bounds = GlobalBox(c_RootNodeAnchor, -c_RootNodeAnchor);

    Node m_Root;

  public:
    Octree();

    /*
      \returns The leaf node that contains the given index.
               Will return nullptr if index is outside of octree bounds.
    */
    std::shared_ptr<Data> find(const GlobalIndex& index);

  private:
    std::shared_ptr<Data> findImpl(Node& branch, const GlobalIndex& index);
  };

  class DrawCommand : public eng::IndexedDrawCommand<DrawCommand, GlobalIndex>
  {
  public:
    DrawCommand(const GlobalIndex& anchor);

    u32 vertexCount() const;

    eng::mem::IndexData indexData() const;
    eng::mem::Data vertexData() const;
    void clearData();
  };
}