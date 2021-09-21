#include "ENpch.h"
#include "Renderer.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  /*
    Renderer 2D data
  */
  static Shared<VertexArray> cubeVertexArray;
  static Shared<Shader> textureShader;
  static Shared<Texture2D> whiteTexture;



  void Renderer::Initialize()
  {
    EN_PROFILE_FUNCTION();

    RenderCommand::Initialize();
    Renderer2D::Initialize();

    cubeVertexArray = VertexArray::Create();

    float vertices[24 * 5] = {  // Front face
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
                               -0.5f, -0.5f, -2.0f, 0.0f, 1.0f  };

    Shared<VertexBuffer> cubeVB = VertexBuffer::Create(vertices, sizeof(vertices));
    cubeVB->setLayout({ { ShaderDataType::Float3, "a_Position" },
                        { ShaderDataType::Float2, "a_TexCoord" } });
    cubeVertexArray->addVertexBuffer(cubeVB);

    uint32_t indices[36];
    for (int face = 0; face < 6; ++face)
    {
      indices[6 * face + 0] = 0 + 4 * face;
      indices[6 * face + 1] = 1 + 4 * face;
      indices[6 * face + 2] = 2 + 4 * face;

      indices[6 * face + 3] = 2 + 4 * face;
      indices[6 * face + 4] = 3 + 4 * face;
      indices[6 * face + 5] = 0 + 4 * face;
    }
    Shared<IndexBuffer> squareIB = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
    cubeVertexArray->setIndexBuffer(squareIB);

    whiteTexture = Texture2D::Create(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    whiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

    textureShader = Shader::Create("assets/shaders/CubeTexture.glsl");
    textureShader->bind();
    textureShader->setInt("u_Texture", 0);
  }

  void Renderer::Shutdown()
  {
  }

  void Renderer::BeginScene(Camera& camera)
  {
    EN_PROFILE_FUNCTION();

    textureShader->bind();
    textureShader->setMat4("u_ViewProjection", camera.getViewPerspectiveMatrix());
  }

  void Renderer::EndScene()
  {
  }

  void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, Shared<Texture2D> texture)
  {
    EN_PROFILE_FUNCTION();

    texture == nullptr ? whiteTexture->bind() : texture->bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
      * glm::scale(glm::mat4(1.0f), { size.x, size.y, size.z });
    textureShader->setMat4("u_Transform", transform);

    cubeVertexArray->bind();
    RenderCommand::DrawIndexed(cubeVertexArray);
  }

  void Renderer::OnWindowResize(uint32_t width, uint32_t height)
  {
    EN_PROFILE_FUNCTION();

    RenderCommand::SetViewport(0, 0, width, height);
  }
}