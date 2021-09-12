#include "ENpch.h"
#include "Renderer.h"
#include "RenderCommand.h"

namespace Engine
{
  Renderer::SceneData* Renderer::m_SceneData = new Renderer::SceneData;

  void Renderer::BeginScene(OrthographicCamera& camera)
  {
    m_SceneData->viewProjectionMatrix = camera.getViewProjectionMatrix();
  }

  void Renderer::EndScene()
  {
  }

  void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray)
  {
    shader->bind();
    shader->uploadUniformMat4(m_SceneData->viewProjectionMatrix, "u_ViewProjection");

    vertexArray->bind();
    RenderCommand::DrawIndexed(vertexArray);
  }
}