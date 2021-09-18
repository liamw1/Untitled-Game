#pragma once
#include "RendererAPI.h"
#include "Shader.h"
#include "OrthographicCamera.h"

namespace Engine
{
  namespace Renderer
  {
    void Initialize();
    void Shutdown();

    void OnWindowResize(uint32_t width, uint32_t height);

    void BeginScene(OrthographicCamera& camera);
    void EndScene();

    void Submit(const Shared<Shader>& shader, const Shared<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

    inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
  };
}