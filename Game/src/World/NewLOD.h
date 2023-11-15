#pragma once
#include "Indexing/Definitions.h"

namespace newLod
{
  struct Data
  {
  };

  class Node
  {
    static constexpr i32 c_NumChildren = 8;
    static constexpr i32 c_MaxNodeDepth = 8;

    GlobalIndex m_Anchor;
    globalIndex_t m_Depth;

    std::unique_ptr<Node[]> m_Children;
    std::unique_ptr<Data> m_Data;

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

    void split();
    void combine();

    static constexpr i32 MaxDepth() { return c_MaxNodeDepth; }
  };

  class Octree
  {
    static constexpr u64 c_RootNodeSize = eng::math::pow2<u64>(Node::MaxDepth());
    static constexpr GlobalIndex c_RootNodeAnchor = -eng::arithmeticCast<globalIndex_t>(c_RootNodeSize / 2) * GlobalIndex(1);

    Node m_Root;

  public:
    Octree();
  };



  class DrawCommand : public eng::MultiDrawIndexedCommand<GlobalIndex, DrawCommand>
  {
  public:
    DrawCommand(const GlobalIndex& anchor);

    i32 vertexCount() const;
    const void* indexData();
    const void* vertexData();
    void prune() {}

  private:
  };
}