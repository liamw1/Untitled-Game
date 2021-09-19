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

  void Renderer2D::DrawQuad(const QuadParams& params)
  {
    EN_PROFILE_FUNCTION();

    s_Data->textureShader->setFloat4("u_Color", params.color);
    s_Data->textureShader->setFloat("u_ScalingFactor", params.textureScalingFactor);
    params.texture == nullptr ? s_Data->whiteTexture->bind() : params.texture->bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), params.position) 
      * glm::scale(glm::mat4(1.0f), { params.size.x, params.size.y, 1.0f });
    s_Data->textureShader->setMat4("u_Transform", transform);

    s_Data->quadVertexArray->bind();
    RenderCommand::DrawIndexed(s_Data->quadVertexArray);
  }

  void Renderer2D::DrawRotatedQuad(const QuadParams& params, radians rotation)
  {
    EN_PROFILE_FUNCTION();

    s_Data->textureShader->setFloat4("u_Color", params.color);
    s_Data->textureShader->setFloat("u_ScalingFactor", params.textureScalingFactor);
    params.texture == nullptr ? s_Data->whiteTexture->bind() : params.texture->bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), params.position)
      * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
      * glm::scale(glm::mat4(1.0f), { params.size.x, params.size.y, 1.0f });
    s_Data->textureShader->setMat4("u_Transform", transform);

    s_Data->quadVertexArray->bind();
    RenderCommand::DrawIndexed(s_Data->quadVertexArray);
  }
}