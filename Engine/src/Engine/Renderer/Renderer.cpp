#include "ENpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "Uniform.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components.h"
#include "Engine/Debug/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>

namespace eng::renderer
{
  struct CameraUniformData
  {
    math::FMat4 viewProjection;
    math::Float3 cameraPosition;
  };

  struct WireVertex
  {
    math::Float3 position;
    math::Float4 color;
  };

  struct QuadVertex
  {
    math::Float3 position;
    math::Float4 tintColor;
    math::Float2 texCoord;
    int textureIndex;
    float scalingFactor;

    // Editor-only
    int entityID;
  };

  /*
    Renderer data
  */
  static std::unique_ptr<VertexArray> s_CubeVertexArray;
  static std::unique_ptr<VertexArray> s_WireFrameVertexArray;
  static std::unique_ptr<Shader> s_TextureShader;
  static std::unique_ptr<Uniform> s_CameraUniform;
  static std::unique_ptr<Shader> s_WireFrameShader;
  static std::unique_ptr<Texture> s_WhiteTexture;

  static CameraUniformData s_CameraUniformData;

  static constexpr math::Vec3 c_UpDirection(0, 0, 1);
  
  static constexpr math::Float4 c_CubeFrameVertexPositions[8] = { { -0.5f, -0.5f, -0.5f, 1.0f },
                                                                  {  0.5f, -0.5f, -0.5f, 1.0f },
                                                                  {  0.5f, -0.5f,  0.5f, 1.0f },
                                                                  { -0.5f, -0.5f,  0.5f, 1.0f },
                                                                  {  0.5f,  0.5f, -0.5f, 1.0f },
                                                                  { -0.5f,  0.5f, -0.5f, 1.0f },
                                                                  { -0.5f,  0.5f,  0.5f, 1.0f },
                                                                  {  0.5f,  0.5f,  0.5f, 1.0f } };

  static constexpr uint32_t c_CubeFrameIndices[24] = { 0, 1, 1, 2, 2, 3, 3, 0,
                                                       4, 5, 5, 6, 6, 7, 7, 4,
                                                       1, 4, 2, 7, 0, 5, 3, 6 };

  static constexpr math::Float4 c_CubeVertexPositions[6][4] = { { { -0.5f, -0.5f, -0.5f, 1.0f },
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

  static constexpr math::Float2 c_CubeTexCoords[6][4] = { { { 0.0f, 0.0f },
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

  void initialize()
  {
    EN_PROFILE_FUNCTION();

    command::setBlendFunc();

    s_CameraUniform = Uniform::Create(0, sizeof(CameraUniformData));
    
    /* Wire Frame Initialization */
    s_WireFrameVertexArray = VertexArray::Create();
    s_WireFrameVertexArray->setLayout({ { ShaderDataType::Float3, "a_Position"  },
                                        { ShaderDataType::Float4, "a_Color"     } });
    s_WireFrameVertexArray->setIndexBuffer(IndexBuffer(c_CubeFrameIndices, 24));

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
    s_CubeVertexArray->setIndexBuffer(IndexBuffer(cubeIndices, 36));

    /* Texture Initialization */
    s_WhiteTexture = Texture::Create(1, 1);
    uint32_t whiteTextureData = 0xffffffff;
    s_WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

    s_TextureShader = Shader::Create("../Engine/assets/shaders/Quad.glsl");
    s_WireFrameShader = Shader::Create("../Engine/assets/shaders/WireFrame.glsl");
  }

  void shutdown()
  {
  }

  void beginScene(Entity viewer)
  {
    command::setBlending(true);
    command::setDepthTesting(true);

    s_CameraUniformData.viewProjection = scene::CalculateViewProjection(viewer);
    s_CameraUniformData.cameraPosition = viewer.get<component::Transform>().position;
    s_CameraUniform->set(&s_CameraUniformData, sizeof(CameraUniformData));
  }

  void endScene()
  {
  }

  void drawCube(const math::Vec3& position, const math::Vec3& size, const Texture* texture)
  {
    texture == nullptr ? s_WhiteTexture->bind() : texture->bind();

    std::array<QuadVertex, 24> vertices{};
    math::Mat4 transform = glm::translate(math::Mat4(1.0), position) * glm::scale(math::Mat4(1.0), size);
    for (int faceID = 0; faceID < 6; ++faceID)
      for (int i = 0; i < 4; ++i)
      {
        int vertexIndex = 4 * faceID + i;
        vertices[vertexIndex].position = transform * c_CubeVertexPositions[faceID][i];
        vertices[vertexIndex].tintColor = math::Float4(1.0f);
        vertices[vertexIndex].texCoord = c_CubeTexCoords[faceID][i];
        vertices[vertexIndex].textureIndex = 0;
        vertices[vertexIndex].scalingFactor = 1.0f;
        vertices[vertexIndex].entityID = -1;
      }
    s_CubeVertexArray->setVertexBuffer(vertices.data(), vertices.size() * sizeof(QuadVertex));

    s_TextureShader->bind();
    command::drawIndexed(s_CubeVertexArray.get());
  }

  void drawCubeFrame(const math::Vec3& position, const math::Vec3& size, const math::Float4& color)
  {
    std::array<WireVertex, 8> vertices{};
    math::Mat4 transform = glm::translate(math::Mat4(1.0), position) * glm::scale(math::Mat4(1.0), size);
    for (int i = 0; i < 8; ++i)
    {
      vertices[i].position = transform * c_CubeFrameVertexPositions[i];
      vertices[i].color = color;
    }
    s_WireFrameVertexArray->setVertexBuffer(vertices.data(), vertices.size() * sizeof(WireVertex));

    s_WireFrameShader->bind();
    command::drawIndexedLines(s_WireFrameVertexArray.get());
  }

  void onWindowResize(uint32_t width, uint32_t height)
  {
    command::setViewport(0, 0, width, height);
  }
}