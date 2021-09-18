#pragma once
#include "OrthographicCamera.h"
#include "Texture.h"

namespace Engine
{
  namespace Renderer2D
  {
    void Init();
    void Shutdown();

    void BeginScene(const OrthographicCamera& camera);
    void EndScene();

    // Primitives
    void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
    void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Shared<Texture2D>& texture);
    void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Shared<Texture2D>& texture);
  };
}