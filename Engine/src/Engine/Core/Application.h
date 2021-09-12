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
    std::unique_ptr<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    LayerStack m_LayerStack;
    bool m_Running = true;
    std::chrono::system_clock::time_point m_LastFrameTime = std::chrono::system_clock::now();

    bool onWindowClose(WindowCloseEvent& event);
  };

  // To be defined in CLIENT
  Application* CreateApplication();
}