#pragma once
#include "World/World.h"

class GameSandbox : public eng::Layer
{
  World m_World;
  std::list<f32> m_FrameTimeWindow;
  bool m_PrintFrameRate;
  bool m_PrintMinFrameRate;
  bool m_PrintPlayerPosition;

public:
  GameSandbox();
  ~GameSandbox();

  void onAttach() override;
  void onDetach() override;

  void onUpdate(eng::Timestep timestep) override;
  void onEvent(eng::event::Event& event) override;

private:
  bool onKeyPress(eng::event::KeyPress& event);
};