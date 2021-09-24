#pragma once
#include <Engine.h>
#include "Block/BlockIDs.h"

class Chunk
{
public:
  Chunk(Block blockType);
  ~Chunk();

private:
  static constexpr uint8_t s_ChunkSize = 32;
  static constexpr uint32_t s_ChunkVolume = s_ChunkSize * s_ChunkSize * s_ChunkSize;
  std::array<Block, s_ChunkVolume> chunkComposition;
};