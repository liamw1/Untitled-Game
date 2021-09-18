#pragma once
#include "Engine/Core/Layer.h"

namespace Engine
{
  class ImGuiLayer : public Layer
  {
  public:
    ImGuiLayer();
    ~ImGuiLayer();

    void onAttach() override;
    void onDetach() override;

    void begin();
    void end();

  private:
    float m_Time = 0.0f;
  };
}