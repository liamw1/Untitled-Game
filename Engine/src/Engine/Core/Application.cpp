#include "ENpch.h"
#include "Application.h"

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

namespace Engine
{
  Application* Application::Instance = nullptr;

  Application::Application()
  {
    EN_CORE_ASSERT(Instance == nullptr, "Application already exists!");
    Instance = this;

    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->setEventCallback(BIND_EVENT_FN(onEvent));
  }

  Application::~Application()
  {
  }

  void Application::run()
  {
    while (m_Running)
    {
      for (Layer* layer : m_LayerStack)
        layer->onUpdate();

      m_Window->onUpdate();
    }
  }

  void Application::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT_FN(onWindowClose));

    EN_CORE_TRACE("{0}", event);

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