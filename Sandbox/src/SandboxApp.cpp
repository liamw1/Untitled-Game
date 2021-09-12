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
      m_CameraPosition({0.0f, 0.0f, 0.0f}),
      m_TrianglePosition({ 0.0f, 0.0f, 0.0f })
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
      uniform mat4 u_Transform;

      out vec3 v_Position;
      out vec4 v_Color;

      void main()
      {
        v_Position = a_Position + 0.5;
        v_Color = a_Color;
        gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
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

    std::string flatColorVertexSource = R"(
      #version 330 core

      layout(location = 0) in vec3 a_Position;

      uniform mat4 u_ViewProjection;
      uniform mat4 u_Transform;

      out vec3 v_Position;

      void main()
      {
        v_Position = a_Position;
        gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
      }
    )";
    std::string flatColorFragmentSource = R"(
      #version 330 core

      layout(location = 0) out vec4 color;

      in vec3 v_Position;

      uniform vec3 u_Color;

      void main()
      {
        color = vec4(u_Color, 1.0);
      }
    )";

    m_Shader = std::shared_ptr<Engine::Shader>(Engine::Shader::Create(flatColorVertexSource, flatColorFragmentSource));
  }

  void onUpdate(std::chrono::duration<float> timestep) override
  {
    const float dt = timestep.count();  // Time between frames in seconds
    EN_TRACE("dt: {0}ms", dt * 1000.0f);

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

    if (Engine::Input::IsKeyPressed(Key::J))
      m_TrianglePosition.x -= m_TriangleMoveSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::L))
      m_TrianglePosition.x += m_TriangleMoveSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::I))
      m_TrianglePosition.y += m_TriangleMoveSpeed * dt;
    if (Engine::Input::IsKeyPressed(Key::K))
      m_TrianglePosition.y -= m_TriangleMoveSpeed * dt;

    Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

    m_Camera.setPosition(m_CameraPosition);
    m_Camera.setRotation(m_CameraRotation);

    Engine::Renderer::BeginScene(m_Camera);

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

    std::dynamic_pointer_cast<Engine::OpenGLShader>(m_Shader)->bind();
    std::dynamic_pointer_cast<Engine::OpenGLShader>(m_Shader)->uploadUniformFloat3("u_Color", m_TriangleColor);

    for (int i = 0; i < 20; ++i)
    {
      for (int j = 0; j < 20; ++j)
      {
        glm::vec3 pos(i * 0.11f, j * 0.11f, 0.0f);
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
        Engine::Renderer::Submit(m_Shader, m_VertexArray, transform);
      }
    }

    Engine::Renderer::EndScene();
  }

  void onImGuiRender() override
  {
    ImGui::Begin("Settings");
    ImGui::ColorEdit3("Square Color", glm::value_ptr(m_TriangleColor));
    ImGui::End();
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

  glm::vec3 m_TrianglePosition;
  float m_TriangleMoveSpeed = 1.0f;

  glm::vec3 m_TriangleColor = { 0.2f, 0.3, 0.8f };
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