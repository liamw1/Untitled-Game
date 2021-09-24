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
    float textureIndex;
    float scalingFactor;
  };

  /*
    Renderer 2D data
  */
  static constexpr glm::vec4 s_QuadVertexPositions[4] = { { -0.5f, -0.5f, 0.0f, 1.0f },
                                                          {  0.5f, -0.5f, 0.0f, 1.0f },
                                                          {  0.5f,  0.5f, 0.0f, 1.0f },
                                                          { -0.5f,  0.5f, 0.0f, 1.0f } };

  static constexpr glm::vec2 s_QuadTextureCoordinates[4] = { {0.0f, 0.0f},
                                                             {1.0f, 0.0f},
                                                             {1.0f, 1.0f},
                                                             {0.0f, 1.0f} };

  // Maximum values per draw call
  static constexpr uint32_t s_MaxQuads = 10000;
  static constexpr uint32_t s_MaxVertices = 4 * s_MaxQuads;
  static constexpr uint32_t s_MaxIndices = 6 * s_MaxQuads;
  static constexpr uint32_t s_MaxTextureSlots = 32;   // TODO: RenderCapabilities

  static Shared<VertexArray> s_QuadVertexArray;
  static Shared<VertexBuffer> s_QuadVertexBuffer;
  static Shared<Shader> s_TextureShader;
  static Shared<Texture2D> s_WhiteTexture;

  static uint32_t s_QuadIndexCount = 0;
  static QuadVertex* s_QuadVertexBufferBase = nullptr;
  static QuadVertex* s_QuadVertexBufferPtr = nullptr;

  static std::array<Shared<Texture2D>, s_MaxTextureSlots> s_TextureSlots;
  static uint32_t s_TextureSlotIndex = 1;   // 0 = white texture

  static Renderer2D::Statistics s_Stats;



  static void flushAndReset()
  {
    Renderer2D::EndScene();

    s_QuadIndexCount = 0;
    s_QuadVertexBufferPtr = s_QuadVertexBufferBase;

    s_TextureSlotIndex = 1;
  }

  static float getTextureIndex(const Shared<Texture2D>& texture)
  {
    float textureIndex = 0.0f;  // White texture index by default
    if (texture != nullptr)
    {
      for (uint32_t i = 1; i < s_TextureSlotIndex; ++i)
        if (*s_TextureSlots[i].get() == *texture.get())
        {
          textureIndex = (float)i;
          break;
        }

      if (textureIndex == 0.0f)
      {
        if (s_TextureSlotIndex >= s_MaxTextureSlots)
          flushAndReset();

        textureIndex = (float)s_TextureSlotIndex;
        s_TextureSlots[s_TextureSlotIndex] = texture;
        s_TextureSlotIndex++;
      }
    }

    return textureIndex;
  }



  void Renderer2D::Initialize()
  {
    EN_PROFILE_FUNCTION();

    s_QuadVertexArray = VertexArray::Create();

    s_QuadVertexBuffer = VertexBuffer::Create(s_MaxVertices * sizeof(QuadVertex));
    s_QuadVertexBuffer->setLayout({ { ShaderDataType::Float3, "a_Position" },
                                    { ShaderDataType::Float4, "a_Color" },
                                    { ShaderDataType::Float2, "a_TexCoord" },
                                    { ShaderDataType::Float,  "a_TextureIndex" },
                                    { ShaderDataType::Float,  "a_ScalingFactor" } });
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

    int samplers[s_MaxTextureSlots];
    for (int i = 0; i < s_MaxTextureSlots; ++i)
      samplers[i] = i;

    s_TextureShader = Shader::Create("assets/shaders/Texture.glsl");
    s_TextureShader->bind();
    s_TextureShader->setIntArray("u_Textures", samplers, s_MaxTextureSlots);

    s_TextureSlots[0] = s_WhiteTexture;
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

    s_TextureSlotIndex = 1;
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
    if (s_QuadIndexCount == 0)
      return; // Nothing to draw

    // Bind textures
    for (uint32_t i = 0; i < s_TextureSlotIndex; ++i)
      s_TextureSlots[i]->bind(i);

    RenderCommand::DrawIndexed(s_QuadVertexArray, s_QuadIndexCount);
    s_Stats.drawCalls++;
  }

  void Renderer2D::DrawQuad(const QuadParams& params)
  {
    if (s_QuadIndexCount >= s_MaxIndices)
      flushAndReset();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), params.position)
      * glm::scale(glm::mat4(1.0f), { params.size.x, params.size.y, 1.0f });

    float textureIndex = getTextureIndex(params.texture);

    for (int i = 0; i < 4; ++i)
    {
      s_QuadVertexBufferPtr->position = transform * s_QuadVertexPositions[i];
      s_QuadVertexBufferPtr->color = params.tintColor;
      s_QuadVertexBufferPtr->texCoord = s_QuadTextureCoordinates[i];
      s_QuadVertexBufferPtr->textureIndex = textureIndex;
      s_QuadVertexBufferPtr->scalingFactor = params.textureScalingFactor;
      s_QuadVertexBufferPtr++;
    }

    s_QuadIndexCount += 6;
    s_Stats.quadCount++;
  }

  void Renderer2D::DrawRotatedQuad(const QuadParams& params, radians rotation)
  {
    if (s_QuadIndexCount >= s_MaxIndices)
      flushAndReset();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), params.position)
      * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
      * glm::scale(glm::mat4(1.0f), { params.size.x, params.size.y, 1.0f });

    float textureIndex = getTextureIndex(params.texture);

    for (int i = 0; i < 4; ++i)
    {
      s_QuadVertexBufferPtr->position = transform * s_QuadVertexPositions[i];
      s_QuadVertexBufferPtr->color = params.tintColor;
      s_QuadVertexBufferPtr->texCoord = s_QuadTextureCoordinates[i];
      s_QuadVertexBufferPtr->textureIndex = textureIndex;
      s_QuadVertexBufferPtr->scalingFactor = params.textureScalingFactor;
      s_QuadVertexBufferPtr++;
    }

    s_QuadIndexCount += 6;
    s_Stats.quadCount++;
  }



  Renderer2D::Statistics Renderer2D::GetStats()
  {
    return s_Stats;
  }

  void Renderer2D::ResetStats()
  {
    s_Stats = Statistics();
  }
}