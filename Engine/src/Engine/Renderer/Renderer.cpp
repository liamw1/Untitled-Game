#include "ENpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "Engine/Scene/Scene.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  /*
    Renderer data
  */
  static Unique<VertexArray> s_CubeVertexArray;
  static Unique<VertexArray> s_CubeFrameVertexArray;
  static Unique<Shader> s_TextureShader;
  static Unique<Shader> s_CubeFrameShader;
  static Unique<Texture2D> s_WhiteTexture;
  
  static constexpr float s_CubeFrameVertices[24 * 3] = { -0.5f, -0.5f, -0.5f,
                                                          0.5f, -0.5f, -0.5f,
                                                          0.5f, -0.5f,  0.5f,
                                                         -0.5f, -0.5f,  0.5f,
                                                          0.5f,  0.5f, -0.5f,
                                                         -0.5f,  0.5f, -0.5f,
                                                         -0.5f,  0.5f,  0.5f,
                                                          0.5f,  0.5f,  0.5f };

  static constexpr uint32_t s_CubeFrameIndices[24] = { 0, 1, 1, 2, 2, 3, 3, 0,
                                                       4, 5, 5, 6, 6, 7, 7, 4,
                                                       1, 4, 2, 7, 0, 5, 3, 6 };

  static constexpr float s_CubeVertices[24 * 5] = {  // Front face
                                                    -0.5f, -0.5f, -2.0f, 0.0f, 0.0f,
                                                     0.5f, -0.5f, -2.0f, 1.0f, 0.0f,
                                                     0.5f,  0.5f, -2.0f, 1.0f, 1.0f,
                                                    -0.5f,  0.5f, -2.0f, 0.0f, 1.0f,

                                                     // Right face
                                                     0.5f, -0.5f, -2.0f, 0.0f, 0.0f,
                                                     0.5f, -0.5f, -3.0f, 1.0f, 0.0f,
                                                     0.5f,  0.5f, -3.0f, 1.0f, 1.0f,
                                                     0.5f,  0.5f, -2.0f, 0.0f, 1.0f,

                                                     // Back face
                                                     0.5f, -0.5f, -3.0f, 0.0f, 0.0f,
                                                    -0.5f, -0.5f, -3.0f, 1.0f, 0.0f,
                                                    -0.5f,  0.5f, -3.0f, 1.0f, 1.0f,
                                                     0.5f,  0.5f, -3.0f, 0.0f, 1.0f,

                                                     // Left face
                                                    -0.5f, -0.5f, -3.0f, 0.0f, 0.0f,
                                                    -0.5f, -0.5f, -2.0f, 1.0f, 0.0f,
                                                    -0.5f,  0.5f, -2.0f, 1.0f, 1.0f,
                                                    -0.5f,  0.5f, -3.0f, 0.0f, 1.0f,

                                                     // Top face
                                                    -0.5f,  0.5f, -2.0f, 0.0f, 0.0f,
                                                     0.5f,  0.5f, -2.0f, 1.0f, 0.0f,
                                                     0.5f,  0.5f, -3.0f, 1.0f, 1.0f,
                                                    -0.5f,  0.5f, -3.0f, 0.0f, 1.0f,

                                                     // Bottom face
                                                    -0.5f, -0.5f, -3.0f, 0.0f, 0.0f,
                                                     0.5f, -0.5f, -3.0f, 1.0f, 0.0f,
                                                     0.5f, -0.5f, -2.0f, 1.0f, 1.0f,
                                                    -0.5f, -0.5f, -2.0f, 0.0f, 1.0f };



  void Renderer::Initialize()
  {
    EN_PROFILE_FUNCTION();
    
    /* Cube Frame Initialization */
    s_CubeFrameVertexArray = VertexArray::Create();
    s_CubeFrameVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position" } });
    s_CubeFrameVertexArray->setVertexData(s_CubeFrameVertices, sizeof(s_CubeFrameVertices));
    s_CubeFrameVertexArray->setIndexBuffer(s_CubeFrameIndices, sizeof(s_CubeFrameIndices) / sizeof(uint32_t));

    s_CubeFrameShader = Shader::Create("../Engine/assets/shaders/CubeFrame.glsl");

    /* Cube Initialization */
    s_CubeVertexArray = VertexArray::Create();
    s_CubeVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position" },
                        { ShaderDataType::Float2, "a_TexCoord" } });
    s_CubeVertexArray->setVertexData(s_CubeVertices, sizeof(s_CubeVertices));

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
    s_CubeVertexArray->setIndexBuffer(cubeIndices, sizeof(cubeIndices) / sizeof(uint32_t));

    /* Texture Initialization */
    s_WhiteTexture = Texture2D::Create(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    s_WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

    s_TextureShader = Shader::Create("../Engine/assets/shaders/CubeTexture.glsl");
    s_TextureShader->bind();
    s_TextureShader->setInt("u_Texture", 0);
  }

  void Renderer::Shutdown()
  {
  }

  void Renderer::BeginScene(const Mat4& viewProjection)
  {
    s_CubeFrameShader->bind();
    s_CubeFrameShader->setMat4("u_ViewProjection", viewProjection);

    s_TextureShader->bind();
    s_TextureShader->setMat4("u_ViewProjection", viewProjection);
  }

  void Renderer::EndScene()
  {
  }

  void Renderer::DrawCube(const Vec3& position, const Vec3& size, Shared<Texture2D> texture)
  {
    texture == nullptr ? s_WhiteTexture->bind() : texture->bind();

    Mat4 transform = glm::translate(Mat4(1.0), position) * glm::scale(Mat4(1.0), size);
    s_TextureShader->setMat4("u_Transform", transform);

    s_CubeVertexArray->bind();
    RenderCommand::DrawIndexed(*s_CubeVertexArray);
  }

  void Renderer::DrawCubeFrame(const Vec3& position, const Vec3& size, const Float4& color)
  {
    s_CubeFrameShader->bind();

    Mat4 transform = glm::translate(Mat4(1.0), position) * glm::scale(Mat4(1.0), size);
    s_CubeFrameShader->setMat4("u_Transform", transform);
    s_CubeFrameShader->setFloat4("u_Color", color);

    s_CubeFrameVertexArray->bind();
    RenderCommand::DrawIndexedLines(*s_CubeFrameVertexArray);
  }

  void Renderer::OnWindowResize(uint32_t width, uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }

  void Renderer::UploadMesh(Unique<VertexArray>& target, const BufferLayout& bufferLayout, const void* data, uintptr_t dataSize, const uint32_t* meshIndices, uint32_t indexCount)
  {
    // Generate vertex array
    Unique<VertexArray> vertexArray = VertexArray::Create();
    vertexArray->setLayout(bufferLayout);
    vertexArray->setVertexData(data, dataSize);
    vertexArray->setIndexBuffer(meshIndices, indexCount);

    target = std::move(vertexArray);
  }
}