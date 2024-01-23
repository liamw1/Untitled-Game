#pragma once
#include "Indexing/Definitions.h"
#include "Block/Block.h"

namespace terrain
{
  length_t getApproximateElevation(const eng::math::Vec2& pointXY);
  block::Type getApproximateBlockType(const eng::math::Vec2& pointXY);

  BlockArrayBox<block::Type> generateNew(const GlobalIndex& chunkIndex);
}