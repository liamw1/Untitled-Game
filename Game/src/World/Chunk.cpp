#include "Chunk.h"

Chunk::Chunk(Block blockType)
{
  for (int i = 0; i < s_ChunkVolume; ++i)
    chunkComposition[i] = blockType;
}

Chunk::~Chunk()
{
}
