#include "ENpch.h"
#include "Renderer2D.h"
#include "VertexArray.h"
#include "Shader.h"
#include "RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  struct Renderer2DData
  {
    Shared<VertexArray> quadVertexArray;
    Shared<Shader> textureShader;
    Shared<Texture2D> whiteTexture;
  };

  static Renderer2DData* s_Data;

  void Renderer2D::Initialize()
  {
    EN_PROFILE_FUNCTION();

    s_Data = new Renderer2DData();

    s_Data->quadVertexArray = VertexArray::Create();

    float vertices[4 * 5] = { -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
                               0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
                               0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
                              -0.5f,  0.5f, 0.0f, 0.0f, 1.0f };

    Shared<VertexBuffer> squareVB = VertexBuffer::Create(vertices, sizeof(vertices));
    squareVB->setLayout({ { ShaderDataType::Float3, "a_Position" },
                          { ShaderDataType::Float2, "a_TexCoord" } });
    s_Data->quadVertexArray->addVertexBuffer(squareVB);

    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
    Shared<IndexBuffer> squareIB = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
    s_Data->quadVertexArray->setIndexBuffer(squareIB);

    s_Data->whiteTexture = Texture2D::Create(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    s_Data->whiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

    s_Data->textureShader = Shader::Create("assets/shaders/Texture.glsl");
    s_Data->textureShader->bind();
    s_Data->textureShader->setInt("u_Texture", 0);
  }

  void Renderer2D::Shutdown()
  {
    delete s_Data;
  }

  void Renderer2D::BeginScene(const OrthographicCamera& camera)
  {
    EN_PROFILE_FUNCTION();

    s_Data->textureShader->bind();
    s_Data->textureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
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
    EN_PROFILE_FUNCTION();

    s_Data->textureShader->setFloat4("u_Color", color);
    s_Data->whiteTexture->bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
    s_Data->textureShader->setMat4("u_Transform", transform);

    s_Data->quadVertexArray->bind();
    RenderCommand::DrawIndexed(s_Data->quadVertexArray);
  }

  void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Shared<Texture2D>& texture)
  {
    DrawQuad({ position.x, position.y, 0.0f }, size, texture);
  }

  void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Shared<Texture2D>& texture)
  {
    EN_PROFILE_FUNCTION();

    s_Data->textureShader->setFloat4("u_Color", glm::vec4(1.0f));
    texture->bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
    s_Data->textureShader->setMat4("u_Transform", transform);

    s_Data->quadVertexArray->bind();
    RenderCommand::DrawIndexed(s_Data->quadVertexArray);
  }
}