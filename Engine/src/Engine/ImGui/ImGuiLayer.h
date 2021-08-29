#pragma once
#include "ENpch.h"
#include "Engine/Core/Layer.h"

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
  };
}