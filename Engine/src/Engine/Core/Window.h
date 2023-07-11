#pragma once
#include "Engine/Events/Event.h"

/*
  Abstract representation of a window.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  struct WindowProps
  {
    std::string title;
    uint32_t width;
    uint32_t height;

    WindowProps(const std::string& title = "Engine", uint32_t width = 1600, uint32_t height = 900)
      : title(title), width(width), height(height) {}
  };

  // Interface representing a desktop system based window
  class Window
  {
  public:
    using EventCallbackFn = std::function<void(Event&)>;

    virtual ~Window() {}

    virtual void onUpdate() = 0;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;

    // Window attributes
    virtual void setEventCallback(const EventCallbackFn& callback) = 0;
    virtual void setVSync(bool enabled) = 0;
    virtual bool isVSync() const = 0;

    virtual void enableCursor() = 0;
    virtual void disableCursor() = 0;

    virtual void* getNativeWindow() const = 0;

    static std::unique_ptr<Window> Create(const WindowProps& properties = WindowProps());
  };
}