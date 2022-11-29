#include "GMpch.h"
#include "Biome.h"
#include "BiomeDefs.h"

const Biome& Biome::Get(Type type)
{
  switch (type)
  {
    case Biome::Type::Default:    return s_DefaultBiome;
    case Biome::Type::GrassField: return s_GrassField;
    case Biome::Type::Desert:     return s_Desert;
    default: EN_ERROR("Unknown biome type!"); return s_DefaultBiome;
  }
}