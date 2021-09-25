#include <Engine.h>
#include "Block/BlockIDs.h"

namespace ChunkRenderer
{
  struct BlockFaceParams
  {
    uint8_t normal;
    glm::vec3 centerPosition;
  };

  void Initialize();
  void Shutdown();

  void BeginScene(Engine::Camera& camera);
  void EndScene();

  // Primitives
  void DrawBlockFace(const BlockFaceParams& params, const glm::vec3& chunkPosition);
};