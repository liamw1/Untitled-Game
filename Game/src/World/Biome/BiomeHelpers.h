#pragma once
#include "Biome.h"

#include <Engine.h>

namespace biome
{
  struct TableNode
  {
    eng::UniqueArray<TableNode, 2> children;
    f64 dividingPlane;
    Property dividingAxis;
    ID biome;

    TableNode();
  };
}
