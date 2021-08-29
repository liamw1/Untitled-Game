#pragma once
#include "ENpch.h"
#include "Engine/Core/Layer.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/ApplicationEvent.h"

namespace Engine
{
  class ENGINE_API ImGuiLayer : public Layer
  {
  public:
    ImGuiLayer();
    ~ImGuiLayer();

    void onAttach();
    void onDetach();
    void onUpdate();
    void onEvent(Event& event);

  private:
    float m_Time = 0.0f;

    bool onMouseButtonPressEvent(MouseButtonPressEvent& event);
    bool onMouseButtonReleaseEvent(MouseButtonReleaseEvent& event);
    bool onMouseMoveEvent(MouseMoveEvent& event);
    bool onMouseScrollEvent(MouseScrollEvent& event);
    bool onKeyPressEvent(KeyPressEvent& event);
    bool onKeyReleaseEvent(KeyReleaseEvent& event);
    bool onKeyTypeEvent(KeyTypeEvent& event);
    bool onWindowResizeEvent(WindowResizeEvent& event);
  };
}