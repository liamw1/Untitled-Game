#pragma once
#include "ENpch.h"
#include "Engine/Core/Window.h"
#include <GLFW/glfw3.h>

namespace Engine
{
  class WindowsWindow : public Window
  {
  public:
    WindowsWindow(const WindowProps& properties);
    ~WindowsWindow();

    void onUpdate() override;

    inline unsigned int getWidth() const override { return m_Data.width; }
    inline unsigned int getHeight() const override { return m_Data.height; }

    // Window attributes
    inline void setEventCallback(const EventCallbackFn& callback) override { m_Data.eventCallback = callback; }
    void setVSync(bool enabled) override;
    bool isVSync() const override;

    inline void* getNativeWindow() const override { return m_Window; }

  private:
    struct WindowData
    {
      std::string title;
      unsigned int width, height;
      bool VSync;

      EventCallbackFn eventCallback;
    };

    WindowData m_Data;
    GLFWwindow* m_Window;

    void initialize(const WindowProps& properties);
    void shutdown();
  };
}