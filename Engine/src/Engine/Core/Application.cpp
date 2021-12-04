#include "ENpch.h"
#include "Application.h"
#include "Engine/Renderer/Renderer.h"

namespace Engine
{
  Application* Application::s_Instance = nullptr;

  Application::Application()
  {
    EN_PROFILE_FUNCTION();

    EN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
    s_Instance = this;

    m_Window = Unique<Window>(Window::Create());
    m_Window->setEventCallback(EN_BIND_EVENT_FN(onEvent));
    m_Window->setVSync(true);

    m_ImGuiLayer = new ImGuiLayer();
    pushOverlay(m_ImGuiLayer);
  }

  Application::~Application()
  {
    EN_PROFILE_FUNCTION();

    Renderer::Shutdown();
  }

  void Application::run()
  {
    EN_PROFILE_FUNCTION();

    while (m_Running)
    {
      EN_PROFILE_SCOPE("Run Loop");

      auto time = std::chrono::steady_clock::now();
      std::chrono::duration<seconds> timestep = time - m_LastFrameTime;
      m_LastFrameTime = time;

      if (!m_Minimized)
      {
        for (Layer* layer : m_LayerStack)
          layer->onUpdate(timestep);

        /*
        m_ImGuiLayer->begin();
        for (Layer* layer : m_LayerStack)
          layer->onImGuiRender();
        m_ImGuiLayer->end();
        */
      }
      
      m_Window->onUpdate();
    }
  }

  void Application::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowCloseEvent>(EN_BIND_EVENT_FN(onWindowClose));
    dispatcher.dispatch<WindowResizeEvent>(EN_BIND_EVENT_FN(onWindowResize));

    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
      if (event.handled)
        break;
      (*it)->onEvent(event);
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

  bool Application::onWindowClose(WindowCloseEvent& /*event*/)
  {
    m_Running = false;
    return true;
  }

  bool Application::onWindowResize(WindowResizeEvent& event)
  {
    if (event.getWidth() == 0 || event.getHeight() == 0)
    {
      m_Minimized = true;
      return false;
    }
    m_Minimized = false;
    Renderer::OnWindowResize(event.getWidth(), event.getHeight());

    return false;
  }
}