#pragma once
#include <Engine.h>

namespace param
{
  constexpr i32 RenderDistance() { return 4; }
  constexpr i32 LoadDistance() { return RenderDistance() + 1; }
  constexpr i32 UnloadDistance() { return LoadDistance(); }

  constexpr length_t BlockLength() { return 0.5_m; }
  constexpr i32 ChunkSize() { return 32; }
  constexpr i32 MaxNodeDepth() { return 28; }
}