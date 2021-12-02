#include "ENpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  /*
    Renderer 2D data
  */
  static Shared<VertexArray> s_CubeVertexArray;
  static Shared<VertexArray> s_CubeFrameVertexArray;
  static Shared<Shader> s_TextureShader;
  static Shared<Shader> s_CubeFrameShader;
  static Shared<Texture2D> s_WhiteTexture;
  
  static constexpr float s_CubeFrameVertices[24 * 3] = { -0.5f, -0.5f, -0.5f,
                                                          0.5f, -0.5f, -0.5f,
                                                          0.5f, -0.5f,  0.5f,
                                                         -0.5f, -0.5f,  0.5f,
                                                          0.5f,  0.5f, -0.5f,
                                                         -0.5f,  0.5f, -0.5f,
                                                         -0.5f,  0.5f,  0.5f,
                                                          0.5f,  0.5f,  0.5f };

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

    Shared<VertexBuffer> cubeFrameVB = VertexBuffer::Create(s_CubeFrameVertices, sizeof(s_CubeFrameVertices));
    cubeFrameVB->setLayout({ { ShaderDataType::Float3, "a_Position" } });
    s_CubeFrameVertexArray->addVertexBuffer(cubeFrameVB);

    uint32_t cubeFrameIndices[24]{};
    cubeFrameIndices[0] = 0;
    cubeFrameIndices[1] = 1;
    cubeFrameIndices[2] = 1;
    cubeFrameIndices[3] = 2;
    cubeFrameIndices[4] = 2;
    cubeFrameIndices[5] = 3;
    cubeFrameIndices[6] = 3;
    cubeFrameIndices[7] = 0;
    cubeFrameIndices[8] = 4;
    cubeFrameIndices[9] = 5;
    cubeFrameIndices[10] = 5;
    cubeFrameIndices[11] = 6;
    cubeFrameIndices[12] = 6;
    cubeFrameIndices[13] = 7;
    cubeFrameIndices[14] = 7;
    cubeFrameIndices[15] = 4;
    cubeFrameIndices[16] = 1;
    cubeFrameIndices[17] = 4;
    cubeFrameIndices[18] = 2;
    cubeFrameIndices[19] = 7;
    cubeFrameIndices[20] = 0;
    cubeFrameIndices[21] = 5;
    cubeFrameIndices[22] = 3;
    cubeFrameIndices[23] = 6;
    Shared<IndexBuffer> cubeFrameIB = IndexBuffer::Create(cubeFrameIndices, sizeof(cubeFrameIndices) / sizeof(uint32_t));
    s_CubeFrameVertexArray->setIndexBuffer(cubeFrameIB);

    s_CubeFrameShader = Shader::Create("assets/shaders/CubeFrame.glsl");

    /* Cube Initialization */
    s_CubeVertexArray = VertexArray::Create();

    Shared<VertexBuffer> cubeVB = VertexBuffer::Create(s_CubeVertices, sizeof(s_CubeVertices));
    cubeVB->setLayout({ { ShaderDataType::Float3, "a_Position" },
                        { ShaderDataType::Float2, "a_TexCoord" } });
    s_CubeVertexArray->addVertexBuffer(cubeVB);

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
    Shared<IndexBuffer> cubeIB = IndexBuffer::Create(cubeIndices, sizeof(cubeIndices) / sizeof(uint32_t));
    s_CubeVertexArray->setIndexBuffer(cubeIB);

    /* Texture Initialization */
    s_WhiteTexture = Texture2D::Create(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    s_WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

    s_TextureShader = Shader::Create("assets/shaders/CubeTexture.glsl");
    s_TextureShader->bind();
    s_TextureShader->setInt("u_Texture", 0);
  }

  void Renderer::Shutdown()
  {
  }

  void Renderer::BeginScene(const Camera& camera)
  {
    s_CubeFrameShader->bind();
    s_CubeFrameShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

    s_TextureShader->bind();
    s_TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());
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
    RenderCommand::DrawIndexed(s_CubeVertexArray);
  }

  void Renderer::DrawCubeFrame(const Vec3& position, const Vec3& size, const Float4& color)
  {
    s_CubeFrameShader->bind();

    Mat4 transform = glm::translate(Mat4(1.0), position) * glm::scale(Mat4(1.0), size);
    s_CubeFrameShader->setMat4("u_Transform", transform);
    s_CubeFrameShader->setFloat4("u_Color", color);

    s_CubeFrameVertexArray->bind();
    RenderCommand::DrawIndexedLines(s_CubeFrameVertexArray);
  }

  void Renderer::OnWindowResize(uint32_t width, uint32_t height)
  {
    RenderCommand::SetViewport(0, 0, width, height);
  }
}