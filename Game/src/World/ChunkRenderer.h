#pragma once
#include <Engine.h>
#include "Block/Block.h"

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

  // Primitives
  void DrawBlockFace(const BlockFaceParams& params, const glm::vec3& chunkPosition);
};