#include "GMpch.h"
#include "BiomeTable.h"

namespace biome
{
  Table::Table(ID defaultBiome)
  {
    m_Root.biome = defaultBiome;
  }

  void Table::addBiome(ID biome, f64 includeElevation, f64 includeTemperature)
  {

  }
}
