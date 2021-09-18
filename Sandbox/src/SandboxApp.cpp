#include <Engine.h>
#include <Engine/Core/EntryPoint.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Sandbox2D.h"

// TEMPORARY
#include <Platform/OpenGL/OpenGLShader.h>

class ExampleLayer : public Engine::Layer
{
public:
  ExampleLayer()
    : Layer("Example"),
      m_CameraController(1280.0f / 720.0f, true)
  {
    m_VertexArray = Engine::VertexArray::Create();

    float vertices[4 * 5] = { -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
                               0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
                               0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
                              -0.5f,  0.5f, 0.0f, 0.0f, 1.0f };

    Engine::Shared<Engine::VertexBuffer> vertexBuffer = Engine::VertexBuffer::Create(vertices, sizeof(vertices));
    Engine::BufferLayout layout = { {ShaderDataType::Float3, "a_Position"},
                                    {ShaderDataType::Float2, "a_TexCoord"} };
    vertexBuffer->setLayout(layout);
    m_VertexArray->addVertexBuffer(vertexBuffer);

    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
    Engine::Shared<Engine::IndexBuffer> indexBuffer = Engine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
    m_VertexArray->setIndexBuffer(indexBuffer);

    auto textureShader = m_ShaderLibrary.load("assets/shaders/Texture.glsl");
    m_Texture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");
    m_LogoTexture = Engine::Texture2D::Create("assets/textures/ChernoLogo.png");

    textureShader->bind();
    textureShader->setInt("u_Texture", 0);
  }

  void onUpdate(std::chrono::duration<float> timestep) override
  {
    const float dt = timestep.count();  // Time between frames in seconds
    // EN_TRACE("dt: {0}ms", dt * 1000.0f);

    m_CameraController.onUpdate(timestep);

    Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

    Engine::Renderer::BeginScene(m_CameraController.getCamera());

    auto textureShader = m_ShaderLibrary.get("Texture");

    m_Texture->bind();
    Engine::Renderer::Submit(textureShader, m_VertexArray);
    m_LogoTexture->bind();
    Engine::Renderer::Submit(textureShader, m_VertexArray);

    Engine::Renderer::EndScene();
  }

  void onImGuiRender() override
  {
  }

  void onEvent(Engine::Event& event) override
  {
    m_CameraController.onEvent(event);
  }

private:
  Engine::ShaderLibrary m_ShaderLibrary;
  Engine::Shared<Engine::VertexArray> m_VertexArray;
  Engine::Shared<Engine::Texture2D> m_Texture, m_LogoTexture;

  Engine::OrthographicCameraController m_CameraController;
};

class Sandbox : public Engine::Application
{
public:
  Sandbox()
  {
    // pushLayer(new ExampleLayer());
    pushLayer(new Sandbox2D());
  }

  ~Sandbox()
  {

  }
};

Engine::Application* Engine::CreateApplication()
{
  return new Sandbox();
}