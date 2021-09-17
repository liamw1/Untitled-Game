#include "ENpch.h"
#include "Renderer2D.h"

#include "VertexArray.h"
#include "Shader.h"
#include "RenderCommand.h"

// TEMPORARY
#include <Platform/OpenGL/OpenGLShader.h>

namespace Engine
{
  struct Renderer2DData
  {
    Shared<VertexArray> quadVertexArray;
    Shared<Shader> flatColorShader;
  };

  static Renderer2DData* s_Data;

  void Renderer2D::Init()
  {
    s_Data = new Renderer2DData();

    s_Data->quadVertexArray = VertexArray::Create();

    float vertices[4 * 3] = { -0.5f, -0.5f, 0.0f,
                               0.5f, -0.5f, 0.0f,
                               0.5f,  0.5f, 0.0f,
                              -0.5f,  0.5f, 0.0f };

    Shared<VertexBuffer> squareVB = VertexBuffer::Create(vertices, sizeof(vertices));
    squareVB->setLayout({ { ShaderDataType::Float3, "a_Position" } });
    s_Data->quadVertexArray->addVertexBuffer(squareVB);

    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
    Shared<IndexBuffer> squareIB = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
    s_Data->quadVertexArray->setIndexBuffer(squareIB);

    s_Data->flatColorShader = Shader::Create("assets/shaders/FlatColor.glsl");
  }

  void Renderer2D::Shutdown()
  {
    delete s_Data;
  }

  void Renderer2D::BeginScene(const OrthographicCamera& camera)
  {
    std::dynamic_pointer_cast<OpenGLShader>(s_Data->flatColorShader)->bind();
    std::dynamic_pointer_cast<OpenGLShader>(s_Data->flatColorShader)->uploadUniformMat4("u_ViewProjection", camera.getViewProjectionMatrix());
    std::dynamic_pointer_cast<OpenGLShader>(s_Data->flatColorShader)->uploadUniformMat4("u_Transform", glm::mat4(1.0f));
  }

  void Renderer2D::EndScene()
  {
  }

  void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
  {
    DrawQuad({ position.x, position.y, 0.0f }, size, color);
  }

  void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
  {
    std::dynamic_pointer_cast<OpenGLShader>(s_Data->flatColorShader)->bind();
    std::dynamic_pointer_cast<OpenGLShader>(s_Data->flatColorShader)->uploadUniformFloat4("u_Color", color);

    s_Data->quadVertexArray->bind();
    RenderCommand::DrawIndexed(s_Data->quadVertexArray);
  }
}