#include <Engine.h>

class ExampleLayer : public Engine::Layer
{
public:
  ExampleLayer()
    : Layer("Example"),
      m_Camera(-1.6f, 1.6f, -0.9f, 0.9f),
      m_CameraPosition({0.0f, 0.0f, 0.0f})
  {
    m_VertexArray = std::shared_ptr<Engine::VertexArray>(Engine::VertexArray::Create());

    float vertices[3 * 7] = { -0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.8f, 1.0f,
                               0.5f, -0.5f, 0.0f, 0.2f, 0.3f, 0.8f, 1.0f,
                               0.0f,  0.5f, 0.0f, 0.8f, 0.8f, 0.2f, 1.0f };

    std::shared_ptr<Engine::VertexBuffer> vertexBuffer = std::shared_ptr<Engine::VertexBuffer>(Engine::VertexBuffer::Create(vertices, sizeof(vertices)));
    Engine::BufferLayout layout = { {ShaderDataType::Float3, "a_Position"},
                                    {ShaderDataType::Float4, "a_Color"} };
    vertexBuffer->setLayout(layout);
    m_VertexArray->addVertexBuffer(vertexBuffer);

    uint32_t indices[3] = { 0, 1, 2 };
    std::shared_ptr<Engine::IndexBuffer> indexBuffer = std::shared_ptr<Engine::IndexBuffer>(Engine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
    m_VertexArray->setIndexBuffer(indexBuffer);

    std::string vertexSource = R"(
      #version 330 core

      layout(location = 0) in vec3 a_Position;
      layout(location = 1) in vec4 a_Color;

      uniform mat4 u_ViewProjection;

      out vec3 v_Position;
      out vec4 v_Color;

      void main()
      {
        v_Position = a_Position + 0.5;
        v_Color = a_Color;
        gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
      }
    )";
    std::string fragmentSource = R"(
      #version 330 core

      layout(location = 0) out vec4 color;

      in vec3 v_Position;
      in vec4 v_Color;

      void main()
      {
        color = vec4(v_Position, 1.0);
        color = v_Color;
      }
    )";

    m_Shader = std::shared_ptr<Engine::Shader>(Engine::Shader::Create(vertexSource, fragmentSource));
  }

  void onUpdate(std::chrono::duration<float> timestep) override
  {
    const float dt = timestep.count();  // Time between frames in seconds

    if (Engine::Input::IsKeyPressed(Key::Left))
      m_CameraPosition.x -= m_CameraMoveSpeed * dt;
    else if (Engine::Input::IsKeyPressed(Key::Right))
      m_CameraPosition.x += m_CameraMoveSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::Up))
      m_CameraPosition.y += m_CameraMoveSpeed * dt;
    else if (Engine::Input::IsKeyPressed(Key::Down))
      m_CameraPosition.y -= m_CameraMoveSpeed * dt;

    if (Engine::Input::IsKeyPressed(Key::A))
      m_CameraRotation += m_CameraRotateSpeed * dt;
    else if (Engine::Input::IsKeyPressed(Key::D))
      m_CameraRotation -= m_CameraRotateSpeed * dt;

    Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

    m_Camera.setPosition(m_CameraPosition);
    m_Camera.setRotation(m_CameraRotation);

    Engine::Renderer::BeginScene(m_Camera);
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
  std::shared_ptr<Engine::VertexArray> m_VertexArray;
  std::shared_ptr<Engine::Shader> m_Shader;

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