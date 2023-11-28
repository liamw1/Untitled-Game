#pragma once
#include "Engine/Core/Window.h"
#include "Engine/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace eng
{
  class WindowsWindow : public Window
  {
    struct WindowData
    {
      std::string title;
      u32 width = 0;
      u32 height = 0;
      bool VSync = false;

      EventCallbackFn eventCallback;
    };

    WindowData m_Data;
    GLFWwindow* m_Window;
    std::unique_ptr<GraphicsContext> m_Context;

  public:
    WindowsWindow(const WindowProps& properties);
    ~WindowsWindow();

    void onUpdate() override;

    u32 width() const override;
    u32 height() const override;

    // Window attributes
    void setEventCallback(const EventCallbackFn& callback) override;
    void setVSync(bool enabled) override;
    bool isVSync() const override;

    void enableCursor() override;
    void disableCursor() override;

    void* getNativeWindow() const override;

  private:
    void initialize(const WindowProps& properties);
    void shutdown();
  };
}