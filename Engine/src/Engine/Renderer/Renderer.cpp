#include "ENpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Debug/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  struct CameraUniforms
  {
    Float4x4 viewProjection;
  };

  struct WireVertex
  {
    Float3 position;
    Float4 color;
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
    Renderer data
  */
  static Unique<VertexArray> s_CubeVertexArray;
  static Unique<VertexArray> s_WireFrameVertexArray;
  static Unique<Shader> s_TextureShader;
  static Unique<Shader> s_WireFrameShader;
  static Unique<Texture2D> s_WhiteTexture;

  static CameraUniforms s_CameraUniforms;
  
  static constexpr Float4 s_CubeFrameVertexPositions[8] = { { -0.5f, -0.5f, -0.5f, 1.0f },
                                                            {  0.5f, -0.5f, -0.5f, 1.0f },
                                                            {  0.5f, -0.5f,  0.5f, 1.0f },
                                                            { -0.5f, -0.5f,  0.5f, 1.0f },
                                                            {  0.5f,  0.5f, -0.5f, 1.0f },
                                                            { -0.5f,  0.5f, -0.5f, 1.0f },
                                                            { -0.5f,  0.5f,  0.5f, 1.0f },
                                                            {  0.5f,  0.5f,  0.5f, 1.0f } };

  static constexpr uint32_t s_CubeFrameIndices[24] = { 0, 1, 1, 2, 2, 3, 3, 0,
                                                       4, 5, 5, 6, 6, 7, 7, 4,
                                                       1, 4, 2, 7, 0, 5, 3, 6 };

  static constexpr Float4 s_CubeVertexPositions[6][4] = { { { -0.5f, -0.5f, -0.5f, 1.0f },
                                                            { -0.5f, -0.5f,  0.5f, 1.0f },
                                                            { -0.5f,  0.5f,  0.5f, 1.0f },
                                                            { -0.5f,  0.5f, -0.5f, 1.0f } },

                                                          { {  0.5f, -0.5f,  0.5f, 1.0f },
                                                            {  0.5f, -0.5f, -0.5f, 1.0f },
                                                            {  0.5f,  0.5f, -0.5f, 1.0f },
                                                            {  0.5f,  0.5f,  0.5f, 1.0f } },

                                                          { { -0.5f, -0.5f, -0.5f, 1.0f },
                                                            {  0.5f, -0.5f, -0.5f, 1.0f },
                                                            {  0.5f, -0.5f,  0.5f, 1.0f },
                                                            { -0.5f, -0.5f,  0.5f, 1.0f } },
                                                          
                                                          { { -0.5f,  0.5f,  0.5f, 1.0f },
                                                            {  0.5f,  0.5f,  0.5f, 1.0f },
                                                            {  0.5f,  0.5f, -0.5f, 1.0f },
                                                            { -0.5f,  0.5f, -0.5f, 1.0f } },

                                                          { {  0.5f, -0.5f, -0.5f, 1.0f },
                                                            { -0.5f, -0.5f, -0.5f, 1.0f },
                                                            { -0.5f,  0.5f, -0.5f, 1.0f },
                                                            {  0.5f,  0.5f, -0.5f, 1.0f } },

                                                          { { -0.5f, -0.5f,  0.5f, 1.0f },
                                                            {  0.5f, -0.5f,  0.5f, 1.0f },
                                                            {  0.5f,  0.5f,  0.5f, 1.0f },
                                                            { -0.5f,  0.5f,  0.5f, 1.0f } } };

  static constexpr Float2 s_CubeTexCoords[6][4] = { { { 0.0f, 0.0f },
                                                      { 1.0f, 0.0f },
                                                      { 1.0f, 1.0f },
                                                      { 0.0f, 1.0f } },

                                                    { { 0.0f, 0.0f },
                                                      { 1.0f, 0.0f },
                                                      { 1.0f, 1.0f },
                                                      { 0.0f, 1.0f } },

                                                    { { 0.0f, 0.0f },
                                                      { 1.0f, 0.0f },
                                                      { 1.0f, 1.0f },
                                                      { 0.0f, 1.0f } },

                                                    { { 0.0f, 0.0f },
                                                      { 1.0f, 0.0f },
                                                      { 1.0f, 1.0f },
                                                      { 0.0f, 1.0f } },

                                                    { { 0.0f, 0.0f },
                                                      { 1.0f, 0.0f },
                                                      { 1.0f, 1.0f },
                                                      { 0.0f, 1.0f } },

                                                    { { 0.0f, 0.0f },
                                                      { 1.0f, 0.0f },
                                                      { 1.0f, 1.0f },
                                                      { 0.0f, 1.0f } } };

  void Renderer::Initialize()
  {
    EN_PROFILE_FUNCTION();

    UniformBuffer::Allocate(0, sizeof(CameraUniforms));
    
    /* Wire Frame Initialization */
    s_WireFrameVertexArray = VertexArray::Create();
    s_WireFrameVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position"  },
                                        { ShaderDataType::Float4, "a_Color"     } });
    s_WireFrameVertexArray->setIndexBuffer(IndexBuffer::Create(s_CubeFrameIndices, 24));

    /* Cube Initialization */
    s_CubeVertexArray = VertexArray::Create();
    s_CubeVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position"      },
                                   { ShaderDataType::Float4, "a_TintColor"     },
                                   { ShaderDataType::Float2, "a_TexCoord"      },
                                   { ShaderDataType::Int,    "a_TextureIndex"  },
                                   { ShaderDataType::Float,  "a_TilingFactor"  },
                                   { ShaderDataType::Int,    "a_EntityID"      } });

    uint32_t cubeIndices[36]{};
    for (int face = 0; face < 6; ++face)
    {
      cubeIndices[6 * face + 0] = 0 + 4 * face;
      cubeIndices[6 * face + 1] = 1 + 4 * face;
      cubeIndices[6 * face + 2] = 2 + 4 * face;

      cubeIndices[6 * face + 3] = 2 + 4 * face;
      cubeIndices[6 * face + 4] = 3 + 4 * face;
      cubeIndices[6 * face + 5] = 0 + 4 * face;
    }
    s_CubeVertexArray->setIndexBuffer(IndexBuffer::Create(cubeIndices, 36));

    /* Texture Initialization */
    s_WhiteTexture = Texture2D::Create(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    s_WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

    s_TextureShader = Shader::Create("../Engine/assets/shaders/Quad.glsl");
    s_WireFrameShader = Shader::Create("../Engine/assets/shaders/WireFrame.glsl");
  }

  void Renderer::Shutdown()
  {
  }

  void Renderer::BeginScene(const Mat4& viewProjection)
  {
    s_CameraUniforms.viewProjection = viewProjection;
    UniformBuffer::SetData(0, &s_CameraUniforms);
  }

  void Renderer::EndScene()
  {
  }

  void Renderer::DrawCube(const Vec3& position, const Vec3& size, const Texture2D* texture)
  {
    texture == nullptr ? s_WhiteTexture->bind() : texture->bind();

    std::array<QuadVertex, 24> vertices{};
    Mat4 transform = glm::translate(Mat4(1.0), position) * glm::scale(Mat4(1.0), size);
    for (int faceID = 0; faceID < 6; ++faceID)
      for (int i = 0; i < 4; ++i)
      {
        int vertexIndex = 4 * faceID + i;
        vertices[vertexIndex].position = transform * s_CubeVertexPositions[faceID][i];
        vertices[vertexIndex].tintColor = Float4(1.0f);
        vertices[vertexIndex].texCoord = s_CubeTexCoords[faceID][i];
        vertices[vertexIndex].textureIndex = 0;
        vertices[vertexIndex].scalingFactor = 1.0f;
        vertices[vertexIndex].entityID = -1;
      }
    s_CubeVertexArray->setVertexBuffer(vertices.data(), vertices.size() * sizeof(QuadVertex));

    s_TextureShader->bind();
    RenderCommand::DrawIndexed(s_CubeVertexArray.get());
  }

  void Renderer::DrawCubeFrame(const Vec3& position, const Vec3& size, const Float4& color)
  {
    std::array<WireVertex, 8> vertices{};
    Mat4 transform = glm::translate(Mat4(1.0), position) * glm::scale(Mat4(1.0), size);
    for (int i = 0; i < 8; ++i)
    {
      vertices[i].position = transform * s_CubeFrameVertexPositions[i];
      vertices[i].color = color;
    }
    s_WireFrameVertexArray->setVertexBuffer(vertices.data(), vertices.size() * sizeof(WireVertex));

    s_WireFrameShader->bind();
    RenderCommand::DrawIndexedLines(s_WireFrameVertexArray.get());
  }

  void Renderer::OnWindowResize(uint32_t width, uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }
}