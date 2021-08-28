#include "ENpch.h"
#include "Application.h"

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

namespace Engine
{

  Application::Application()
  {
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
      m_Window->onUpdate();
    }
  }

  void Application::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT_FN(onWindowClose));

    EN_CORE_TRACE("{0}", event);
  }

  bool Application::onWindowClose(WindowCloseEvent& event)
  {
    m_Running = false;
    return true;
  }
}