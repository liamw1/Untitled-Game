#pragma once
#include "World/World.h"

class GameSandbox : public eng::Layer
{
public:
  GameSandbox();
  ~GameSandbox();

  void onAttach() override;
  void onDetach() override;

  void onUpdate(eng::Timestep timestep) override;
  void onImGuiRender() override;
  void onEvent(eng::event::Event& event) override;

private:
  World m_World;
  std::list<float> m_FrameTimeWindow;
  bool m_PrintFrameRate;
  bool m_PrintMinFrameRate;
  bool m_PrintPlayerPosition;

  bool onKeyPress(eng::event::KeyPress& event);
};