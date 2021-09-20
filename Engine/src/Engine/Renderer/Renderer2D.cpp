#include "ENpch.h"
#include "Renderer2D.h"
#include "VertexArray.h"
#include "Shader.h"
#include "RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  struct QuadVertex
  {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    // TODO: texID
  };


  /*
    Renderer 2D data
  */
  // Maximum values per draw call
  static constexpr uint32_t s_MaxQuads = 10000;
  static constexpr uint32_t s_MaxVertices = 4 * s_MaxQuads;
  static constexpr uint32_t s_MaxIndices = 6 * s_MaxQuads;

  static uint32_t s_QuadIndexCount = 0;
  static QuadVertex* s_QuadVertexBufferBase = nullptr;
  static QuadVertex* s_QuadVertexBufferPtr = nullptr;

  static Shared<VertexArray> s_QuadVertexArray;
  static Shared<VertexBuffer> s_QuadVertexBuffer;
  static Shared<Shader> s_TextureShader;
  static Shared<Texture2D> s_WhiteTexture;



  void Renderer2D::Initialize()
  {
    EN_PROFILE_FUNCTION();

    s_QuadVertexArray = VertexArray::Create();

    s_QuadVertexBuffer = VertexBuffer::Create(s_MaxVertices * sizeof(QuadVertex));
    s_QuadVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                    { ShaderDataType::Float4, "a_Color" },
                                    { ShaderDataType::Float2, "a_TexCoord" } });
    s_QuadVertexArray->addVertexBuffer(s_QuadVertexBuffer);

    s_QuadVertexBufferBase = new QuadVertex[s_MaxVertices];

    uint32_t* quadIndices = new uint32_t[s_MaxIndices];

    uint32_t offset = 0;
    for (uint32_t i = 0; i < s_MaxIndices; i += 6)
    {
      // Triangle 1
      quadIndices[i + 0] = offset + 0;
      quadIndices[i + 1] = offset + 1;
      quadIndices[i + 2] = offset + 2;

      // Triangle 2
      quadIndices[i + 3] = offset + 2;
      quadIndices[i + 4] = offset + 3;
      quadIndices[i + 5] = offset + 0;

      offset += 4;
    }

    Shared<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_MaxIndices);
    s_QuadVertexArray->setIndexBuffer(quadIB);
    delete[] quadIndices;

    s_WhiteTexture = Texture2D::Create(1, 1);
    uint32_t s_WhiteTextureData = 0xffffffff;
    s_WhiteTexture->setData(&s_WhiteTextureData, sizeof(uint32_t));

    s_TextureShader = Shader::Create("assets/shaders/Texture.glsl");
    s_TextureShader->bind();
    s_TextureShader->setInt("u_Texture", 0);
  }

  void Renderer2D::Shutdown()
  {
    delete[] s_QuadVertexBufferBase;
  }

  void Renderer2D::BeginScene(const OrthographicCamera& camera)
  {
    EN_PROFILE_FUNCTION();

    s_TextureShader->bind();
    s_TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

    s_QuadIndexCount = 0;
    s_QuadVertexBufferPtr = s_QuadVertexBufferBase;
  }

  void Renderer2D::EndScene()
  {
    EN_PROFILE_FUNCTION();

    // Cast to 1-byte value to determine number of bytes in vertexBufferPtr
    uintptr_t dataSize = (uint8_t*)s_QuadVertexBufferPtr - (uint8_t*)s_QuadVertexBufferBase;
    s_QuadVertexBuffer->setData(s_QuadVertexBufferBase, dataSize);

    Flush();
  }

  void Renderer2D::Flush()
  {
    RenderCommand::DrawIndexed(s_QuadVertexArray, s_QuadIndexCount);
  }

  void Renderer2D::DrawQuad(const QuadParams& params)
  {
    EN_PROFILE_FUNCTION();

    const glm::vec3& position = params.position;
    const glm::vec2& size = params.size;
    const glm::vec4& color = params.color;
    const float& textureScalingFactor = params.textureScalingFactor;
    const Shared<Texture2D>& texture = params.texture;

    s_QuadVertexBufferPtr->position = position;
    s_QuadVertexBufferPtr->color = color;
    s_QuadVertexBufferPtr->texCoord = { 0.0f, 0.0f };
    s_QuadVertexBufferPtr++;

    s_QuadVertexBufferPtr->position = { position.x + size.x, position.y, position.z };
    s_QuadVertexBufferPtr->color = color;
    s_QuadVertexBufferPtr->texCoord = { 1.0f, 0.0f };
    s_QuadVertexBufferPtr++;

    s_QuadVertexBufferPtr->position = { position.x + size.x, position.y + size.y, position.z };
    s_QuadVertexBufferPtr->color = color;
    s_QuadVertexBufferPtr->texCoord = { 1.0f, 1.0f };
    s_QuadVertexBufferPtr++;

    s_QuadVertexBufferPtr->position = { position.x, position.y + size.y, position.z };
    s_QuadVertexBufferPtr->color = color;
    s_QuadVertexBufferPtr->texCoord = { 0.0f, 1.0f };
    s_QuadVertexBufferPtr++;

    s_QuadIndexCount += 6;

    /*
    s_TextureShader->setFloat("u_ScalingFactor", textureScalingFactor);
    texture == nullptr ? s_WhiteTexture->bind() : texture->bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) 
      * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
    s_TextureShader->setMat4("u_Transform", transform);

    s_QuadVertexArray->bind();
    RenderCommand::DrawIndexed(s_QuadVertexArray);
    */
  }

  void Renderer2D::DrawRotatedQuad(const QuadParams& params, radians rotation)
  {
    EN_PROFILE_FUNCTION();

    s_TextureShader->setFloat4("u_Color", params.color);
    s_TextureShader->setFloat("u_ScalingFactor", params.textureScalingFactor);
    params.texture == nullptr ? s_WhiteTexture->bind() : params.texture->bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), params.position)
      * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
      * glm::scale(glm::mat4(1.0f), { params.size.x, params.size.y, 1.0f });
    s_TextureShader->setMat4("u_Transform", transform);

    s_QuadVertexArray->bind();
    RenderCommand::DrawIndexed(s_QuadVertexArray);
  }
}