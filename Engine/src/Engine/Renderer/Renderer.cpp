#include "ENpch.h"
#include "Renderer.h"
#include "Framebuffer.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "Uniform.h"
#include "Engine/Core/Application.h"
#include "Engine/Debug/Instrumentor.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components.h"

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
  static constexpr i32 c_CameraUniformBinding = 0;
  static std::unique_ptr<Uniform> s_CameraUniform;
  static std::unique_ptr<Shader> s_WireFrameShader;
  static std::unique_ptr<VertexArray> s_WireFrameVertexArray;
  static std::unique_ptr<Framebuffer> s_DefaultFramebuffer;
  static CameraUniformData s_CameraUniformData;
  
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
    static bool initialized = []()
    {
      FramebufferSpecification frameBufferSpecification({ FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::Depth32f });
      frameBufferSpecification.width = Application::Get().window().width();
      frameBufferSpecification.height = Application::Get().window().height();
      s_DefaultFramebuffer = Framebuffer::Create(frameBufferSpecification);

      s_CameraUniform = std::make_unique<Uniform>(c_CameraUniformBinding, sizeof(CameraUniformData));

      /* Wire Frame Initialization */
      s_WireFrameVertexArray = VertexArray::Create();
      s_WireFrameVertexArray->setLayout({ { mem::ShaderDataType::Float3, "a_Position"  },
                                          { mem::ShaderDataType::Float4, "a_Color"     } });
      s_WireFrameVertexArray->setIndexBuffer(IndexBuffer(c_CubeFrameIndices));
      s_WireFrameShader = Shader::Create("../Engine/assets/shaders/WireFrame.glsl");
      return true;
    }();
  }



  void beginScene(Entity viewer)
  {
    initialize();

    s_DefaultFramebuffer->bind();

    command::setDepthTesting(true);

    s_CameraUniformData.viewProjection = scene::CalculateViewProjection(viewer);
    s_CameraUniformData.cameraPosition = viewer.get<component::Transform>().position;
    s_CameraUniform->set(s_CameraUniformData);
  }

  void endScene()
  {
    s_DefaultFramebuffer->copyToWindow();
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
    s_DefaultFramebuffer->resize(width, height);
  }
}