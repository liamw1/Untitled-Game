#include "ENpch.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include "Engine/Scene/Scene.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  struct CameraUniforms
  {
    Float4x4 viewProjection;
  };

  struct QuadVertex
  {
    Float3 position;
    Float4 tintColor;
    Float2 texCoord;
    int textureIndex;
    float scalingFactor;

    // Editor-only
    int entityID;
  };

  /*
    Renderer 2D data
  */
  static constexpr Float4 s_QuadVertexPositions[4] = { { -0.5f, -0.5f, 0.0f, 1.0f },
                                                       {  0.5f, -0.5f, 0.0f, 1.0f },
                                                       {  0.5f,  0.5f, 0.0f, 1.0f },
                                                       { -0.5f,  0.5f, 0.0f, 1.0f } };

  // Maximum values per draw call
  static constexpr uint32_t s_MaxQuads = 10000;
  static constexpr uint32_t s_MaxVertices = 4 * s_MaxQuads;
  static constexpr uint32_t s_MaxIndices = 6 * s_MaxQuads;
  static constexpr uint32_t s_MaxTextureSlots = 32;   // TODO: RenderCapabilities

  static Unique<VertexArray> s_QuadVertexArray;
  static Unique<Shader> s_TextureShader;
  static Shared<Texture2D> s_WhiteTexture;

  static uint32_t s_QuadIndexCount = 0;
  static QuadVertex* s_QuadVertexBufferBase = nullptr;
  static QuadVertex* s_QuadVertexBufferPtr = nullptr;

  static std::array<Shared<Texture2D>, s_MaxTextureSlots> s_TextureSlots;
  static uint32_t s_TextureSlotIndex = 1;   // 0 = white texture

  static Renderer2D::Statistics s_Stats;

  static CameraUniforms s_CameraUniforms;
  static Unique<UniformBuffer> s_CameraUniformBuffer;



  static void startBatch()
  {
    s_QuadIndexCount = 0;
    s_QuadVertexBufferPtr = s_QuadVertexBufferBase;

    s_TextureSlotIndex = 1;
  }
  
  static void nextBatch()
  {
    Renderer2D::Flush();
    startBatch();
  }

  static int getTextureIndex(const Shared<Texture2D>& texture)
  {
    uint32_t textureIndex = 0;  // White texture index by default
    if (texture != nullptr)
    {
      for (uint32_t i = 1; i < s_TextureSlotIndex; ++i)
        if (*s_TextureSlots[i] == *texture)
        {
          textureIndex = i;
          break;
        }

      if (textureIndex == 0)
      {
        if (s_TextureSlotIndex > s_MaxTextureSlots - 1)
          nextBatch();

        textureIndex = s_TextureSlotIndex;
        s_TextureSlots[textureIndex] = texture;
        s_TextureSlotIndex++;
      }
    }

    return textureIndex;
  }



  void Renderer2D::Initialize()
  {
    EN_PROFILE_FUNCTION();

    s_QuadVertexArray = VertexArray::Create();
    s_QuadVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position"      },
                                   { ShaderDataType::Float4, "a_TintColor"     },
                                   { ShaderDataType::Float2, "a_TexCoord"      },
                                   { ShaderDataType::Int,    "a_TextureIndex"  },
                                   { ShaderDataType::Float,  "a_TilingFactor"  },
                                   { ShaderDataType::Int,    "a_EntityID"      } });

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

    s_QuadVertexArray->setIndexBuffer(IndexBuffer::Create(quadIndices, s_MaxIndices));
    delete[] quadIndices;

    s_WhiteTexture = Texture2D::Create(1, 1);
    uint32_t s_WhiteTextureData = 0xFFFFFFFF;
    s_WhiteTexture->setData(&s_WhiteTextureData, sizeof(uint32_t));

    int samplers[s_MaxTextureSlots]{};
    for (int i = 0; i < s_MaxTextureSlots; ++i)
      samplers[i] = i;

    s_TextureShader = Shader::Create("../Engine/assets/shaders/Texture.glsl");

    s_TextureSlots[0] = s_WhiteTexture;

    s_CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraUniforms), 0);
  }

  void Renderer2D::Shutdown()
  {
    delete[] s_QuadVertexBufferBase;
  }

  void Renderer2D::BeginScene(const Mat4& viewProjection)
  {
    s_CameraUniforms.viewProjection = viewProjection;
    s_CameraUniformBuffer->setData(&s_CameraUniforms, sizeof(CameraUniforms));

    startBatch();
  }

  void Renderer2D::EndScene()
  {
    Flush();
  }

  void Renderer2D::Flush()
  {
    if (s_QuadIndexCount == 0)
      return; // Nothing to draw

    uintptr_t dataSize = (s_QuadVertexBufferPtr - s_QuadVertexBufferBase) * sizeof(QuadVertex);
    s_QuadVertexArray->setVertexBuffer(s_QuadVertexBufferBase, dataSize);

    // Bind textures
    for (uint32_t i = 0; i < s_TextureSlotIndex; ++i)
      s_TextureSlots[i]->bind(i);

    s_TextureShader->bind();
    s_QuadVertexArray->bind();
    RenderCommand::DrawIndexed(s_QuadVertexArray.get(), s_QuadIndexCount);
    s_Stats.drawCalls++;
  }

  void Renderer2D::DrawQuad(const Mat4& transform, const Float4& tintColor, float textureScalingFactor, const Shared<Texture2D>& texture, int entityID)
  {
    static constexpr Float2 textureCoordinates[4] = { {0.0f, 0.0f},
                                                      {1.0f, 0.0f},
                                                      {1.0f, 1.0f},
                                                      {0.0f, 1.0f} };

    if (s_QuadIndexCount >= s_MaxIndices)
      nextBatch();

    for (int i = 0; i < 4; ++i)
    {
      s_QuadVertexBufferPtr->position = transform * s_QuadVertexPositions[i];
      s_QuadVertexBufferPtr->tintColor = tintColor;
      s_QuadVertexBufferPtr->texCoord = textureCoordinates[i];
      s_QuadVertexBufferPtr->textureIndex = getTextureIndex(texture);
      s_QuadVertexBufferPtr->scalingFactor = textureScalingFactor;
      s_QuadVertexBufferPtr->entityID = entityID;
      s_QuadVertexBufferPtr++;
    }

    s_QuadIndexCount += 6;
    s_Stats.quadCount++;
  }

  void Renderer2D::DrawQuad(const Vec3& position, const Vec2& size, Angle rotation, const Float4& tintColor, float textureScalingFactor, const Shared<Texture2D>& texture, int entityID)
  {
    Mat4 transform = glm::translate(Mat4(1.0), position)
                   * glm::rotate(Mat4(1.0), rotation.rad(), Vec3(0.0, 0.0, 1.0))
                   * glm::scale(Mat4(1.0), Vec3(size, 1.0));

    DrawQuad(transform, tintColor, textureScalingFactor, texture, entityID);
  }

  void Renderer2D::DrawSprite(const Mat4& transform, const Component::SpriteRenderer& sprite, int entityID)
  {
    DrawQuad(transform, sprite.color, 1.0f, nullptr, entityID);
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