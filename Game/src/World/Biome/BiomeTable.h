#pragma once
#include "BiomeHelpers.h"

namespace biome
{
  // C++23: This entire class could probably be made constexpr
  class Table
  {
    std::unique_ptr<TableNode> m_Root;

  public:
    void addBiome(ID biome, const PropertyVector& properties);

    ID at(const PropertyVector& properties) const;

    eng::EnumArray<PropertyBox, ID> linearize() const;

    void debugPrint() const;

  private:
    TableNode& leafNodeAt(const PropertyVector& properties);
    const TableNode& leafNodeAt(const PropertyVector& properties) const;
  };
}