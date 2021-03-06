#include "ENpch.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Debug/Instrumentor.h"

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

  struct CircleVertex
  {
    Float3 position;
    Float4 color;
    float thickness;
    float fade;
    int quadIndex;

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
  static constexpr uint32_t s_MaxQuadVertices = 4 * s_MaxQuads;
  static constexpr uint32_t s_MaxQuadIndices = 6 * s_MaxQuads;
  static constexpr uint32_t s_MaxCircles = 100;
  static constexpr uint32_t s_MaxCircleVertices = 4 * s_MaxCircles;
  static constexpr uint32_t s_MaxCircleIndices = 6 * s_MaxCircles;
  static constexpr uint32_t s_MaxTextureSlots = 32;   // TODO: RenderCapabilities

  static Unique<VertexArray> s_QuadVertexArray;
  static Unique<Shader> s_QuadShader;
  static Shared<Texture2D> s_WhiteTexture;

  static uint32_t s_QuadIndexCount = 0;
  static QuadVertex* s_QuadVertexBufferBase = nullptr;
  static QuadVertex* s_QuadVertexBufferPtr = nullptr;

  static Unique<VertexArray> s_CircleVertexArray;
  static Unique<Shader> s_CircleShader;

  static uint32_t s_CircleIndexCount = 0;
  static CircleVertex* s_CircleVertexBufferBase = nullptr;
  static CircleVertex* s_CircleVertexBufferPtr = nullptr;

  static std::array<Shared<Texture2D>, s_MaxTextureSlots> s_TextureSlots;
  static uint32_t s_TextureSlotIndex = 1;   // 0 = white texture

  static Renderer2D::Statistics s_Stats;

  static CameraUniforms s_CameraUniforms;



  static void startBatch()
  {
    s_QuadIndexCount = 0;
    s_QuadVertexBufferPtr = s_QuadVertexBufferBase;

    s_CircleIndexCount = 0;
    s_CircleVertexBufferPtr = s_CircleVertexBufferBase;

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
        {
          nextBatch();
          s_TextureSlotIndex = 1;
        }

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

    UniformBuffer::Allocate(0, sizeof(CameraUniforms));

    uint32_t* quadIndices = new uint32_t[s_MaxQuadIndices];

    uint32_t offset = 0;
    for (uint32_t i = 0; i < s_MaxQuadIndices; i += 6)
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
    Shared<IndexBuffer> quadIndexBuffer = IndexBuffer::Create(quadIndices, s_MaxQuadIndices);
    delete[] quadIndices;

    s_QuadVertexArray = VertexArray::Create();
    s_QuadVertexArray->setIndexBuffer(quadIndexBuffer);
    s_QuadVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position"      },
                                   { ShaderDataType::Float4, "a_TintColor"     },
                                   { ShaderDataType::Float2, "a_TexCoord"      },
                                   { ShaderDataType::Int,    "a_TextureIndex"  },
                                   { ShaderDataType::Float,  "a_TilingFactor"  },
                                   { ShaderDataType::Int,    "a_EntityID"      } });
    s_QuadShader = Shader::Create("../Engine/assets/shaders/Quad.glsl");
    s_QuadVertexBufferBase = new QuadVertex[s_MaxQuadVertices];

    s_CircleVertexArray = VertexArray::Create();
    s_CircleVertexArray->setIndexBuffer(quadIndexBuffer);
    s_CircleVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position"   },
                                     { ShaderDataType::Float4, "a_Color"      },
                                     { ShaderDataType::Float,  "a_Thickness"  },
                                     { ShaderDataType::Float,  "a_Fade"       },
                                     { ShaderDataType::Int,    "a_QuadIndex"  },
                                     { ShaderDataType::Int,    "a_EntityID"   } });
    s_CircleShader = Shader::Create("../Engine/assets/shaders/Circle.glsl");
    s_CircleVertexBufferBase = new CircleVertex[s_MaxCircleVertices];

    s_WhiteTexture = Texture2D::Create(1, 1);
    uint32_t s_WhiteTextureData = 0xFFFFFFFF;
    s_WhiteTexture->setData(&s_WhiteTextureData, sizeof(uint32_t));
    s_TextureSlots[0] = s_WhiteTexture;

    int samplers[s_MaxTextureSlots]{};
    for (int i = 0; i < s_MaxTextureSlots; ++i)
      samplers[i] = i;
  }

  void Renderer2D::Shutdown()
  {
    delete[] s_QuadVertexBufferBase;
  }

  void Renderer2D::BeginScene(const Mat4& viewProjection)
  {
    s_CameraUniforms.viewProjection = viewProjection;
    UniformBuffer::SetData(0, &s_CameraUniforms);

    startBatch();
  }

  void Renderer2D::EndScene()
  {
    Flush();
  }

  void Renderer2D::Flush()
  {
    if (s_QuadIndexCount > 0)
    {
      uintptr_t dataSize = (s_QuadVertexBufferPtr - s_QuadVertexBufferBase) * sizeof(QuadVertex);
      s_QuadVertexArray->setVertexBuffer(s_QuadVertexBufferBase, dataSize);

      // Bind textures
      for (uint32_t i = 0; i < s_TextureSlotIndex; ++i)
        s_TextureSlots[i]->bind(i);

      s_QuadShader->bind();
      RenderCommand::DrawIndexed(s_QuadVertexArray.get(), s_QuadIndexCount);
      s_Stats.drawCalls++;
    }

    if (s_CircleIndexCount > 0)
    {
      uintptr_t dataSize = (s_CircleVertexBufferPtr - s_CircleVertexBufferBase) * sizeof(CircleVertex);
      s_CircleVertexArray->setVertexBuffer(s_CircleVertexBufferBase, dataSize);

      s_CircleShader->bind();
      RenderCommand::DrawIndexed(s_CircleVertexArray.get(), s_CircleIndexCount);
      s_Stats.drawCalls++;
    }
  }

  void Renderer2D::DrawQuad(const Mat4& transform, const Float4& tintColor, float textureScalingFactor, const Shared<Texture2D>& texture, int entityID)
  {
    static constexpr Float2 textureCoordinates[4] = { {0.0f, 0.0f},
                                                      {1.0f, 0.0f},
                                                      {1.0f, 1.0f},
                                                      {0.0f, 1.0f} };

    if (s_QuadIndexCount >= s_MaxQuadIndices)
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

  void Renderer2D::DrawCircle(const Mat4& transform, const Float4& color, float thickness, float fade, int entityID)
  {
    // TODO: Implement for circles
    // if (s_CircleIndexCount >= s_MaxCircleIndices)
    //   nextBatch();

    for (int i = 0; i < 4; ++i)
    {
      s_CircleVertexBufferPtr->position = transform * s_QuadVertexPositions[i];
      s_CircleVertexBufferPtr->color = color;
      s_CircleVertexBufferPtr->thickness = thickness;
      s_CircleVertexBufferPtr->fade = fade;
      s_CircleVertexBufferPtr->quadIndex = i;
      s_CircleVertexBufferPtr->entityID = entityID;
      s_CircleVertexBufferPtr++;
    }

    s_CircleIndexCount += 6;
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