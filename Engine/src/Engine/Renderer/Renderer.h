#pragma once
#include "RendererAPI.h"
#include "Shader.h"
#include "OrthographicCamera.h"

namespace Engine
{
  class Renderer
  {
  public:
    static void BeginScene(OrthographicCamera& camera);
    static void EndScene();

    static void Submit(const Shared<Shader>& shader, const Shared<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

    inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

  private:
    struct SceneData
    {
      glm::mat4 viewProjectionMatrix;
    };

    static SceneData* m_SceneData;
  };
}