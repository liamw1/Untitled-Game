#pragma once
#include "OrthographicCamera.h"
#include "Texture.h"
#include "SubTexture.h"

/*
  A general-purpose 2D renderer.
*/
namespace Engine
{
  namespace Renderer2D
  {
    struct QuadParams
    {
      Vec3 position = Vec3(0.0);
      Vec2 size = Vec2(1.0);
      Float4 tintColor = Float4(1.0);
      float textureScalingFactor = 1.0f;
    };

    void Initialize();
    void Shutdown();

    void BeginScene(const OrthographicCamera& camera);
    void EndScene();
    void Flush();

    // Primitives
    void DrawQuad(const QuadParams& params, const Shared<Texture2D>& texture = nullptr);
    void DrawQuad(const QuadParams& params, const Shared<SubTexture2D>& subTexture);
    void DrawRotatedQuad(const QuadParams& params, radians rotation, const Shared<Texture2D>& texture = nullptr);
    void DrawRotatedQuad(const QuadParams& params, radians rotation, const Shared<SubTexture2D>& subTexture);

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