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

    void onAttach() override;
    void onDetach() override;
    void onImGuiRender() override;

    void begin();
    void end();

  private:
    float m_Time = 0.0f;
  };
}