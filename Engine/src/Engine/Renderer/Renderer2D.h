#pragma once
#include "Texture.h"
#include "Engine/Scene/Components.h"

/*
  A general-purpose 2D renderer.
*/
namespace Engine
{
  namespace Renderer2D
  {
    void Initialize();
    void Shutdown();

    void BeginScene(const Mat4& viewProjection);
    void EndScene();
    void Flush();

    // Primitives
    void DrawQuad(const Mat4& transform, const Float4& tintColor = Float4(1.0), float textureScalingFactor = 1.0f, const Shared<Texture2D>& texture = nullptr, int entityID = -1);
    void DrawQuad(const Vec3& position, const Vec2& size, Angle rotation = Angle(0.0f), const Float4& tintColor = Float4(1.0), float textureScalingFactor = 1.0f, const Shared<Texture2D>& texture = nullptr, int entityID = -1);

    void DrawSprite(const Mat4& transform, const Component::SpriteRenderer& sprite, int entityID = -1);
    void DrawCircle(const Mat4& transform, const Float4& color, float thickness, float fade, int entityID = -1);

    // Stats
    struct Statistics
    {
      uint32_t drawCalls = 0;
      uint32_t quadCount = 0;

      uint32_t getTotalVertexCount() const { return quadCount * 4; }
      uint32_t getTotatlIndexCount() const { return quadCount * 6; }
    };
    Statistics GetStats();
    void ResetStats();
  };
}