#pragma once
#include "Engine/Core/Window.h"
#include "Engine/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace Engine
{
  class WindowsWindow : public Window
  {
  public:
    WindowsWindow(const WindowProps& properties);
    ~WindowsWindow();

    void onUpdate() override;

    uint32_t getWidth() const override;
    uint32_t getHeight() const override;

    // Window attributes
    void setEventCallback(const EventCallbackFn& callback) override;
    void setVSync(bool enabled) override;
    bool isVSync() const override;

    void enableCursor() override;
    void disableCursor() override;

    void* getNativeWindow() const override;

  private:
    struct WindowData
    {
      std::string title;
      uint32_t width = 0;
      uint32_t height = 0;
      bool VSync = false;

      EventCallbackFn eventCallback;
    };

    WindowData m_Data;
    GLFWwindow* m_Window;
    std::unique_ptr<GraphicsContext> m_Context;

    void initialize(const WindowProps& properties);
    void shutdown();
  };
}