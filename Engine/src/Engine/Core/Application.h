#pragma once
#include "ENpch.h"
#include "Window.h"
#include "Engine/Core/LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Events/ApplicationEvent.h"

namespace Engine
{
  class ENGINE_API Application
  {
  public:
    Application();
    virtual ~Application();

    void run();

    void onEvent(Event& event);

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);

    inline Window& getWindow() { return *m_Window; }
    inline static Application& Get() { return *Instance; }

  private:
    bool onWindowClose(WindowCloseEvent& event);

    std::unique_ptr<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    bool m_Running = true;

    LayerStack m_LayerStack;

    static Application* Instance;
  };

  // To be defined in CLIENT
  Application* CreateApplication();
}