#include "GMpch.h"
#include "BiomeTable.h"

namespace biome
{
  static TableBranchNode createNewBranch(const TableLeafNode& leafNode, ID biome, const PropertyVector& properties)
  {
    PropertyVector difference;
    for (Property axis : eng::EnumIterator<Property>())
      difference[axis] = std::abs(leafNode.biomeProperties[axis] - properties[axis]);

    Property dividingAxis = difference.enumFromIterator(eng::algo::maxElement(difference));
    length_t dividingPlane = std::midpoint(leafNode.biomeProperties[dividingAxis], properties[dividingAxis]);

    eng::UniqueArray<TableNode, 2> children(eng::AllocationPolicy::ForOverwrite);
    int leafNodeIndex = leafNode.biomeProperties[dividingAxis] < dividingPlane ? 0 : 1;
    children[leafNodeIndex] = leafNode;
    children[(leafNodeIndex + 1) % 2] = TableLeafNode(properties, biome);

    return TableBranchNode(std::move(children), dividingPlane, dividingAxis);
  }

  void Table::addBiome(ID biome, const PropertyVector& properties)
  {
    if (!m_Root)
    {
      m_Root = std::make_unique<TableNode>(std::in_place_type<TableLeafNode>, properties, biome);
      return;
    }

    TableNode& containingNode = leafNodeAt(properties);
    containingNode = createNewBranch(std::get<TableLeafNode>(containingNode), biome, properties);
  }

  ID Table::at(const PropertyVector& properties) const
  {
    if (!m_Root)
      throw eng::Exception("No biomes in biome table!");
    return std::get<TableLeafNode>(leafNodeAt(properties)).biome;
  }

  eng::EnumArray<PropertyBox, ID> Table::linearize() const
  {
    if (!m_Root)
      throw eng::Exception("No biomes in biome table!");

    eng::EnumArray<PropertyBox, ID> linearizedTree(-1_m, 1_m);

    auto recurse = [&linearizedTree](const TableNode& node, PropertyBox biomeBox, const auto& recurse)
    {
      if (const TableLeafNode* leafNode = std::get_if<TableLeafNode>(&node))
      {
        linearizedTree[leafNode->biome] = biomeBox;
        return;
      }

      const TableBranchNode& branchNode = std::get<TableBranchNode>(node);
      eng::math::Interval<length_t> dividingAxisBounds = biomeBox[branchNode.dividingAxis];

      biomeBox[branchNode.dividingAxis] = eng::math::Interval<length_t>(dividingAxisBounds.min, branchNode.dividingPlane);
      recurse(branchNode.children[0], biomeBox, recurse);

      biomeBox[branchNode.dividingAxis] = eng::math::Interval<length_t>(branchNode.dividingPlane, dividingAxisBounds.max);
      recurse(branchNode.children[1], biomeBox, recurse);
    };
    recurse(*m_Root, linearizedTree.front(), recurse);

    // Validation
    for (const PropertyBox& biomeBox : linearizedTree)
      for (const eng::math::Interval<length_t>& propertyBounds : biomeBox)
        if (propertyBounds.length() <= 0)
          throw eng::Exception("Biome properties have no volume!");

    return linearizedTree;
  }

  void Table::debugPrint() const
  {
    if (!m_Root)
    {
      std::cout << "No biomes in biome table\n";
      return;
    }

    const auto& linearizedTree = linearize();
    for (ID biome : eng::EnumIterator<ID>())
    {
      std::cout << "Biome " << eng::enumIndex(biome) << "\n";
      for (Property property : eng::EnumIterator<Property>())
        std::cout << "   Property " << eng::enumIndex(property) << ": " << linearizedTree[biome][property] << "\n";
    }
    std::cout << "\n";
  }

  TableNode& Table::leafNodeAt(const PropertyVector& properties) { ENG_MUTABLE_VERSION(leafNodeAt, properties); }
  const TableNode& Table::leafNodeAt(const PropertyVector& properties) const
  {
    if (!m_Root)
      throw eng::Exception("No biomes in biome table!");

    auto get = [&properties](const TableNode& node, const auto& add) -> const TableNode&
    {
      if (std::holds_alternative<TableLeafNode>(node))
        return node;

      const TableBranchNode& branchnode = std::get<TableBranchNode>(node);
      switch (eng::resultOf(properties[branchnode.dividingAxis] <=> branchnode.dividingPlane))
      {
        case eng::PartialOrdering::Equivalent:
        case eng::PartialOrdering::Less:        return add(branchnode.children[0], add);
        case eng::PartialOrdering::Greater:     return add(branchnode.children[1], add);
        case eng::PartialOrdering::Unordered:   throw eng::Exception("NaN found while adding biome to biome table!");
        default:                                throw eng::Exception("Unknown ordering!");
      }
    };
    return get(*m_Root, get);
  }
}
