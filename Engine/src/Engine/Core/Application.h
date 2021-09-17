#pragma once
#include "Window.h"
#include "LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Events/ApplicationEvent.h"

namespace Engine
{
  class Application
  {
  public:
    Application();
    virtual ~Application();

    void run();

    void onEvent(Event& event);

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);

    inline Window& getWindow() { return *m_Window; }
    inline static Application& Get() { return *s_Instance; }

  private:
    static Application* s_Instance;
    Unique<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    LayerStack m_LayerStack;
    bool m_Running = true;
    bool m_Minimized = false;
    std::chrono::system_clock::time_point m_LastFrameTime = std::chrono::system_clock::now();

    bool onWindowClose(WindowCloseEvent& event);
    bool onWindowResize(WindowResizeEvent& event);
  };

  // To be defined in CLIENT
  Application* CreateApplication();
}