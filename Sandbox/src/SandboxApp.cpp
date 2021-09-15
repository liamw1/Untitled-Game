#include <Engine.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

// TEMPORARY
#include <Platform/OpenGL/OpenGLShader.h>

class ExampleLayer : public Engine::Layer
{
public:
  ExampleLayer()
    : Layer("Example"),
      m_Camera(-1.6f, 1.6f, -0.9f, 0.9f),
      m_CameraPosition({0.0f, 0.0f, 0.0f})
  {
    m_VertexArray = Engine::Shared<Engine::VertexArray>(Engine::VertexArray::Create());

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

    m_Shader = Engine::Shader::Create("assets/shaders/Texture.glsl");
    m_Texture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");
    m_LogoTexture = Engine::Texture2D::Create("assets/textures/ChernoLogo.png");

    std::dynamic_pointer_cast<Engine::OpenGLShader>(m_Shader)->bind();
    std::dynamic_pointer_cast<Engine::OpenGLShader>(m_Shader)->uploadUniformInt("u_Texture", 0);
  }

  void onUpdate(std::chrono::duration<float> timestep) override
  {
    const float dt = timestep.count();  // Time between frames in seconds
    // EN_TRACE("dt: {0}ms", dt * 1000.0f);

    if (Engine::Input::IsKeyPressed(Key::Left))
      m_CameraPosition.x -= m_CameraMoveSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::Right))
      m_CameraPosition.x += m_CameraMoveSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::Up))
      m_CameraPosition.y += m_CameraMoveSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::Down))
      m_CameraPosition.y -= m_CameraMoveSpeed * dt;

    if (Engine::Input::IsKeyPressed(Key::A))
      m_CameraRotation += m_CameraRotateSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::D))
      m_CameraRotation -= m_CameraRotateSpeed * dt;

    Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

    m_Camera.setPosition(m_CameraPosition);
    m_Camera.setRotation(m_CameraRotation);

    Engine::Renderer::BeginScene(m_Camera);

    m_Texture->bind();
    Engine::Renderer::Submit(m_Shader, m_VertexArray);
    m_LogoTexture->bind();
    Engine::Renderer::Submit(m_Shader, m_VertexArray);

    Engine::Renderer::EndScene();
  }

  void onImGuiRender() override
  {
  }

  void onEvent(Engine::Event& event) override
  {
    Engine::EventDispatcher dispatcher(event);
  }

private:
  Engine::Shared<Engine::VertexArray> m_VertexArray;
  Engine::Shared<Engine::Shader> m_Shader;
  Engine::Shared<Engine::Texture2D> m_Texture;
  Engine::Shared<Engine::Texture2D> m_LogoTexture;

  Engine::OrthographicCamera m_Camera;
  glm::vec3 m_CameraPosition;
  float m_CameraRotation = 0.0f;

  float m_CameraMoveSpeed = 3.0f;
  float m_CameraRotateSpeed = 180.0f;
};

class Sandbox : public Engine::Application
{
public:
  Sandbox()
  {
    pushLayer(new ExampleLayer());
  }

  ~Sandbox()
  {

  }
};

Engine::Application* Engine::CreateApplication()
{
  return new Sandbox();
}