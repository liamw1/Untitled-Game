#pragma once
#include "Engine/Events/Event.h"

/*
  Abstract representation of a window.
  Platform-specific implementation is determined by derived class.
*/
namespace eng
{
  struct WindowProps
  {
    std::string title;
    u32 width;
    u32 height;

    WindowProps(std::string_view title = "Engine", u32 width = 1600, u32 height = 900)
      : title(title), width(width), height(height) {}
  };

  // Interface representing a desktop system based window
  class Window
  {
  public:
    using EventCallbackFn = std::function<void(event::Event&)>;

    virtual ~Window() {}

    virtual void onUpdate() = 0;

    virtual u32 width() const = 0;
    virtual u32 height() const = 0;

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