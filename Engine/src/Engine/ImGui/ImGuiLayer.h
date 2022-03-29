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
    void onEvent(Event& event) override;

    void begin();
    void end();

    void blockEvents(bool isBlocking) { m_BlockEvents = isBlocking; }

  private:
    bool m_BlockEvents = true;

    void setDarkThemeColors();
  };
}