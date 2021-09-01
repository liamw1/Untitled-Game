#include "ENpch.h"
#include "Application.h"
#include "Input.h"

// TEMPORARY
#include <glad/glad.h>

namespace Engine
{
  Application* Application::s_Instance = nullptr;

  Application::Application()
  {
    EN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
    s_Instance = this;

    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->setEventCallback(EN_BIND_EVENT_FN(onEvent));

    m_ImGuiLayer = new ImGuiLayer();
    pushOverlay(m_ImGuiLayer);

    glGenVertexArrays(1, &m_VertexArray);
    glBindVertexArray(m_VertexArray);

    glGenBuffers(1, &m_VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);

    float vertices[3 * 3] = { -0.5f, -0.5f, 0.0f,
                               0.5f, -0.5f, 0.0f,
                               0.0f,  0.5f, 0.0f };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glGenBuffers(1, &m_IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);

    unsigned int indices[3] = { 0, 1, 2 };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    std::string vertexSource = R"(
      #version 330 core

      layout(location = 0) in vec3 a_Position;

      out vec3 v_Position;

      void main()
      {
        v_Position = a_Position + 0.5;
        gl_Position = vec4(a_Position, 1.0);
      }
    )";
    std::string fragmentSource = R"(
      #version 330 core

      layout(location = 0) out vec4 color;

      in vec3 v_Position;

      void main()
      {
        color = vec4(v_Position, 1.0);
      }
    )";

    m_Shader = std::make_unique<Shader>(vertexSource, fragmentSource);
  }

  Application::~Application()
  {
  }

  void Application::run()
  {
    while (m_Running)
    {
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      m_Shader->bind();
      glBindVertexArray(m_VertexArray);
      glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

      for (Layer* layer : m_LayerStack)
        layer->onUpdate();

      m_ImGuiLayer->begin();
      for (Layer* layer : m_LayerStack)
        layer->onImGuiRender();
      m_ImGuiLayer->end();

      m_Window->onUpdate();
    }
  }

  void Application::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
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

  bool Application::onWindowClose(WindowCloseEvent& event)
  {
    m_Running = false;
    return true;
  }
}