#pragma once
#include "Indexing/Definitions.h"
#include "Block/Block.h"
#include "World/Biome/BiomeHelpers.h"

namespace terrain
{
  biome::PropertyVector terrainPropertiesAt(const eng::math::Vec2& pointXY);

  biome::ID biomeAt(const biome::PropertyVector& terrainProperties);
  block::Type getApproximateBlockType(biome::ID biome);
  length_t getApproximateElevation(const eng::math::Vec2& pointXY);

  BlockArrayBox<block::Type> generateNew(const GlobalIndex& chunkIndex);
}