#pragma once
#include "Window.h"
#include "LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Log.h"

// Entry point
int main(int argc, char** argv);

namespace Engine
{
  struct ApplicationCommandLineArgs
  {
    int count = 0;
    char** args = nullptr;

    const char* operator[](int index) const;
  };

  class Application
  {
  public:
    Application(const std::string& name = "Engine", ApplicationCommandLineArgs args = ApplicationCommandLineArgs());
    virtual ~Application();

    // Handler for all player input
    void onEvent(Event& event);

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);

    void close();

    ImGuiLayer* getImGuiLayer();
    Window& getWindow();
    const ApplicationCommandLineArgs& GetCommandLineArgs() const;
    static Application& Get();

  private:
    static Application* s_Instance;
    ApplicationCommandLineArgs m_CommandLineArgs;
    std::unique_ptr<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    std::unique_ptr<LayerStack> m_LayerStack;
    bool m_Running;
    bool m_Minimized;
    std::chrono::steady_clock::time_point m_LastFrameTime;

    void run();

    bool onWindowClose(WindowCloseEvent& event);
    bool onWindowResize(WindowResizeEvent& event);

    friend int ::main(int argc, char** argv);
  };

  // To be defined in CLIENT
  Application* CreateApplication(ApplicationCommandLineArgs args);
}