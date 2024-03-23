#pragma once
#include "BiomeHelpers.h"

namespace biome
{
  class Table
  {
    static constexpr i32 c_Dimensions = eng::enumCount<Property>();

    TableNode m_Root;

  public:
    Table(ID defaultBiome);

    void addBiome(ID biome, f64 includeElevation, f64 includeTemperature);

  private:
  };
}