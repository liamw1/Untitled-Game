#pragma once
#include "OrthographicCamera.h"
#include "Texture.h"

namespace Engine
{
  namespace Renderer2D
  {
    struct QuadParams
    {
      glm::vec3 position = glm::vec3(0.0f);
      glm::vec2 size = glm::vec2(1.0f);
      glm::vec4 tintColor = glm::vec4(1.0f);
      float textureScalingFactor = 1.0f;

      Shared<Texture2D> texture = nullptr;  // Texture is all-white by default
    };

    void Initialize();
    void Shutdown();

    void BeginScene(const OrthographicCamera& camera);
    void EndScene();
    void Flush();

    // Primitives
    void DrawQuad(const QuadParams& params);
    void DrawRotatedQuad(const QuadParams& params, radians rotation);

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
}