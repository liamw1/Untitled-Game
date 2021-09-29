#pragma once
#include <Engine.h>
#include "Block/Block.h"

class Chunk;

/*
  A specialized renderer optimized for chunk rendering.
*/
namespace ChunkRenderer
{
  struct BlockFaceParams
  {
    BlockFace normal;
    glm::vec3 relativePosition;
  };

  void Initialize();
  void Shutdown();

  void BeginScene(Engine::Camera& camera);
  void EndScene();

  void StartBatch();
  void Flush();

  void DrawBlockFace(const BlockFaceParams& params, const glm::vec3& chunkPosition);
  void DrawChunk(const Chunk& chunk);

  // Stats
  struct Statistics
  {
    uint32_t drawCalls = 0;
    uint32_t quadCount = 0;

    uint32_t getTotalVertexCount() { return quadCount * 4; }
    uint32_t getTotatlIndexCount() { return quadCount * 6; }
  };
  Statistics GetStats();
  void ResetStats();
};