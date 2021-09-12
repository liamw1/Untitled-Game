#include "ENpch.h"
#include "Application.h"
#include "Input.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RenderCommand.h"

namespace Engine
{
  Application* Application::s_Instance = nullptr;

  Application::Application()
    : m_Camera(-1.6f, 1.6f, -0.9f, 0.9f)
  {
    EN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
    s_Instance = this;

    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->setEventCallback(EN_BIND_EVENT_FN(onEvent));

    m_ImGuiLayer = new ImGuiLayer();
    pushOverlay(m_ImGuiLayer);

    m_VertexArray = std::shared_ptr<VertexArray>(VertexArray::Create());

    float vertices[3 * 7] = { -0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.8f, 1.0f,
                               0.5f, -0.5f, 0.0f, 0.2f, 0.3f, 0.8f, 1.0f,
                               0.0f,  0.5f, 0.0f, 0.8f, 0.8f, 0.2f, 1.0f };

    std::shared_ptr<VertexBuffer> vertexBuffer = std::shared_ptr<VertexBuffer>(VertexBuffer::Create(vertices, sizeof(vertices)));
    BufferLayout layout = { {ShaderDataType::Float3, "a_Position"},
                            {ShaderDataType::Float4, "a_Color"} };
    vertexBuffer->setLayout(layout);
    m_VertexArray->addVertexBuffer(vertexBuffer);

    uint32_t indices[3] = { 0, 1, 2 };
    std::shared_ptr<IndexBuffer> indexBuffer = std::shared_ptr<IndexBuffer>(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
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

    m_Shader = std::shared_ptr<Shader>(Shader::Create(vertexSource, fragmentSource));
  }

  Application::~Application()
  {
  }

  void Application::run()
  {
    while (m_Running)
    {
      RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

      Renderer::BeginScene(m_Camera);
      Renderer::Submit(m_Shader, m_VertexArray);
      Renderer::EndScene();

      for (Layer* layer : m_LayerStack)
        layer->onUpdate();

      /*
      m_ImGuiLayer->begin();
      for (Layer* layer : m_LayerStack)
        layer->onImGuiRender();
      m_ImGuiLayer->end();
      */

      m_Window->onUpdate();
    }
  }

  void Application::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<MouseMoveEvent>(EN_BIND_EVENT_FN(onMouseMove));
    dispatcher.dispatch<WindowCloseEvent>(EN_BIND_EVENT_FN(onWindowClose));

    for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
    {
      (*--it)->onEvent(event);
      if (event.handled)
        break;
    }
  }

  void Application::pushLayer(Layer* layer)
  {
    m_LayerStack.pushLayer(layer);
    layer->onAttach();
  }

  void Application::pushOverlay(Layer* layer)
  {
    m_LayerStack.pushOverlay(layer);
    layer->onAttach();
  }

  bool Application::onMouseMove(MouseMoveEvent& event)
  {
    m_Camera.setPosition({ event.getX() / -400.0f + 1.6f, event.getY() / 400.0f - 1.0f, 0.0f });
    return true;
  }

  bool Application::onWindowClose(WindowCloseEvent& event)
  {
    m_Running = false;
    return true;
  }
}