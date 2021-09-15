#include "ENpch.h"
#include "Renderer.h"
#include "RenderCommand.h"

// TEMPORARY
#include "Platform/OpenGL/OpenGLShader.h"

namespace Engine
{
  Renderer::SceneData* Renderer::m_SceneData = new Renderer::SceneData;

  void Renderer::Init()
  {
    RenderCommand::Init();
  }

  void Renderer::BeginScene(OrthographicCamera& camera)
  {
    m_SceneData->viewProjectionMatrix = camera.getViewProjectionMatrix();
  }

  void Renderer::EndScene()
  {
  }

  void Renderer::Submit(const Shared<Shader>& shader, const Shared<VertexArray>& vertexArray, const glm::mat4& transform)
  {
    shader->bind();
    std::dynamic_pointer_cast<OpenGLShader>(shader)->uploadUniformMat4("u_ViewProjection", m_SceneData->viewProjectionMatrix);
    std::dynamic_pointer_cast<OpenGLShader>(shader)->uploadUniformMat4("u_Transform", transform);

    vertexArray->bind();
    RenderCommand::DrawIndexed(vertexArray);
  }
}