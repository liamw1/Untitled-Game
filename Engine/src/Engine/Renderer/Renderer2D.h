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
      glm::vec4 color = glm::vec4(1.0f);
      float textureScalingFactor = 1.0f;

      Shared<Texture2D> texture = nullptr;  // Texture is all-white by default
    };

    void Initialize();
    void Shutdown();

    void BeginScene(const OrthographicCamera& camera);
    void EndScene();

    // Primitives
    void DrawQuad(const QuadParams& params);
    void DrawRotatedQuad(const QuadParams& params, radians rotation);
  };
}