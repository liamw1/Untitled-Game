#pragma once

namespace biome
{
  enum class ID
  {
    Default,
    Mountain,
    Beach,
    Ocean,
    Abyss,

    First = 0, Last = Abyss
  };

  enum class Property
  {
    Elevation,
    Temperature,

    First = 0, Last = Elevation
  };
}