#pragma once
#include "ENpch.h"
#include "Window.h"
#include "Events/ApplicationEvent.h"

namespace Engine
{
  class ENGINE_API Application
  {
  public:
    Application();
    virtual ~Application();

    void run();

    void onEvent(Event& event);

  private:
    bool onWindowClose(WindowCloseEvent& event);

    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
  };

  // To be defined in CLIENT
  Application* CreateApplication();
}