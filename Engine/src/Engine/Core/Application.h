#pragma once
#include "Window.h"
#include "LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Events/ApplicationEvent.h"

// Entry point
int main(int argc, char** argv);

namespace eng
{
  class ApplicationCommandLineArgs
  {
    i32 m_Count;
    char** m_Args;

  public:
    ApplicationCommandLineArgs();
    ApplicationCommandLineArgs(i32 argCount, char** commandLineArgs);

    std::string_view operator[](i32 index) const;
  };

  class Application
  {
    static Application* s_Instance;
    ApplicationCommandLineArgs m_CommandLineArgs;
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
    LayerStack m_LayerStack;
    bool m_Running;
    bool m_Minimized;
    std::chrono::steady_clock::time_point m_LastFrameTime;

  public:
    Application(std::string_view name = "Engine", ApplicationCommandLineArgs args = ApplicationCommandLineArgs());
    virtual ~Application();

    // Handler for all player input
    void onEvent(event::Event& event);

    void pushLayer(std::unique_ptr<Layer> layer);

    void close();

    ImGuiLayer& imGuiLayer();
    Window& window();
    const ApplicationCommandLineArgs& GetCommandLineArgs() const;
    static Application& Get();

  private:
    void run();

    bool onWindowClose(event::WindowClose& event);
    bool onWindowResize(event::WindowResize& event);

    friend int ::main(int argc, char** argv);
  };

  // To be defined in CLIENT
  Application* createApplication(ApplicationCommandLineArgs args);
}