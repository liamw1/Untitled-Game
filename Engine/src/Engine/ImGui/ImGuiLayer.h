#pragma once
#include "Engine/Core/Layer.h"

namespace eng
{
  class ImGuiLayer : public Layer
  {
  public:
    ImGuiLayer();

    void onAttach() override;
    void onDetach() override;
    void onEvent(event::Event& event) override;

    void begin();
    void end();

    void blockEvents(bool isBlocking);

  private:
    bool m_BlockEvents = true;

    void setDarkThemeColors();
  };
}