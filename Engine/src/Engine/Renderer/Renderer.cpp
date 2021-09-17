#include "ENpch.h"
#include "Renderer.h"
#include "Renderer2D.h"
#include "RenderCommand.h"

// TEMPORARY
#include "Platform/OpenGL/OpenGLShader.h"

namespace Engine
{
  struct SceneData
  {
    glm::mat4 viewProjectionMatrix;
  };

  static SceneData* s_SceneData;

  void Renderer::Init()
  {
    s_SceneData = new SceneData;

    RenderCommand::Init();
    Renderer2D::Init();
  }

  void Renderer::Shutdown()
  {
    delete s_SceneData;
  }

  void Renderer::OnWindowResize(uint32_t width, uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }

  void Renderer::BeginScene(OrthographicCamera& camera)
  {
    s_SceneData->viewProjectionMatrix = camera.getViewProjectionMatrix();
  }

  void Renderer::EndScene()
  {
  }

  void Renderer::Submit(const Shared<Shader>& shader, const Shared<VertexArray>& vertexArray, const glm::mat4& transform)
  {
    shader->bind();
    std::dynamic_pointer_cast<OpenGLShader>(shader)->uploadUniformMat4("u_ViewProjection", s_SceneData->viewProjectionMatrix);
    std::dynamic_pointer_cast<OpenGLShader>(shader)->uploadUniformMat4("u_Transform", transform);

    vertexArray->bind();
    RenderCommand::DrawIndexed(vertexArray);
  }
}