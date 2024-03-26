#pragma once
#include "BiomeHelpers.h"

namespace biome
{
  class Table
  {
    std::unique_ptr<TableNode> m_Root;

  public:
    void addBiome(ID biome, const eng::EnumArray<f64, Property>& properties);

    ID at(const eng::EnumArray<f64, Property>& properties) const;

    eng::EnumArray<eng::EnumArray<eng::math::Interval<f64>, Property>, ID> linearize() const;

    void debugPrint() const;

  private:
    TableNode& leafNodeAt(const eng::EnumArray<f64, Property>& properties);
    const TableNode& leafNodeAt(const eng::EnumArray<f64, Property>& properties) const;
  };
}