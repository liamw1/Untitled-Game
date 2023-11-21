#include "ENpch.h"
#include "Renderer.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "Uniform.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components.h"
#include "Engine/Debug/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>

namespace eng::render
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

  /*
    Renderer data
  */
  static std::unique_ptr<Uniform> s_CameraUniform;
  static std::unique_ptr<Shader> s_WireFrameShader;
  static std::unique_ptr<VertexArray> s_WireFrameVertexArray;
  static CameraUniformData s_CameraUniformData;
  static std::once_flag s_InitializedFlag;
  
  static constexpr math::Float4 c_CubeFrameVertexPositions[8] = { { -0.5f, -0.5f, -0.5f, 1.0f },
                                                                  {  0.5f, -0.5f, -0.5f, 1.0f },
                                                                  {  0.5f, -0.5f,  0.5f, 1.0f },
                                                                  { -0.5f, -0.5f,  0.5f, 1.0f },
                                                                  {  0.5f,  0.5f, -0.5f, 1.0f },
                                                                  { -0.5f,  0.5f, -0.5f, 1.0f },
                                                                  { -0.5f,  0.5f,  0.5f, 1.0f },
                                                                  {  0.5f,  0.5f,  0.5f, 1.0f } };

  static constexpr std::array<u32, 24> c_CubeFrameIndices = { 0, 1, 1, 2, 2, 3, 3, 0,
                                                              4, 5, 5, 6, 6, 7, 7, 4,
                                                              1, 4, 2, 7, 0, 5, 3, 6 };

  static void initialize()
  {
    std::call_once(s_InitializedFlag, []()
    {
      command::setBlendFunc();

      s_CameraUniform = Uniform::Create(0, sizeof(CameraUniformData));

      /* Wire Frame Initialization */
      s_WireFrameVertexArray = VertexArray::Create();
      s_WireFrameVertexArray->setLayout({{ mem::ShaderDataType::Float3, "a_Position"  },
                                         { mem::ShaderDataType::Float4, "a_Color"     }});
      s_WireFrameVertexArray->setIndexBuffer(IndexBuffer(c_CubeFrameIndices));
      s_WireFrameShader = Shader::Create("../Engine/assets/shaders/WireFrame.glsl");
    });
  }



  void beginScene(Entity viewer)
  {
    initialize();

    command::setBlending(true);
    command::setDepthTesting(true);

    s_CameraUniformData.viewProjection = scene::CalculateViewProjection(viewer);
    s_CameraUniformData.cameraPosition = viewer.get<component::Transform>().position;
    s_CameraUniform->set(s_CameraUniformData);
  }

  void endScene()
  {
  }

  void drawCubeFrame(const math::Vec3& position, const math::Vec3& size, const math::Float4& color)
  {
    std::array<WireVertex, 8> vertices{};
    math::Mat4 transform = glm::translate(math::Mat4(1.0), position) * glm::scale(math::Mat4(1.0), size);
    for (i32 i = 0; i < 8; ++i)
    {
      vertices[i].position = transform * c_CubeFrameVertexPositions[i];
      vertices[i].color = color;
    }
    s_WireFrameVertexArray->setVertexBuffer(vertices);

    s_WireFrameShader->bind();
    command::drawIndexedLines(s_WireFrameVertexArray.get());
  }

  void onWindowResize(u32 width, u32 height)
  {
    command::setViewport(0, 0, width, height);
  }
}