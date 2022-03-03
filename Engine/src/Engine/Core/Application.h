#pragma once
#include "Window.h"
#include "LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Events/ApplicationEvent.h"

// Entry point
int main(int argc, char** argv);

namespace Engine
{
  class Application
  {
  public:
    Application(const std::string& name = "Engine");
    virtual ~Application();

    // Handler for all player input
    void onEvent(Event& event);

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);

    void close();

    ImGuiLayer* getImGuiLayer() { return m_ImGuiLayer; }

    Window& getWindow() { return *m_Window; }
    static Application& Get() { return *s_Instance; }

  private:
    static Application* s_Instance;
    Unique<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    LayerStack m_LayerStack;
    bool m_Running = true;
    bool m_Minimized = false;
    std::chrono::steady_clock::time_point m_LastFrameTime = std::chrono::steady_clock::now();

    void run();

    bool onWindowClose(WindowCloseEvent& event);
    bool onWindowResize(WindowResizeEvent& event);

    friend int ::main(int argc, char** argv);
  };

  // To be defined in CLIENT
  Application* CreateApplication();
}