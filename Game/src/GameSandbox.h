#pragma once
#include "World/World.h"

class GameSandbox : public Engine::Layer
{
public:
  GameSandbox();
  ~GameSandbox();

  void onAttach() override;
  void onDetach() override;

  void onUpdate(Engine::Timestep timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  World m_World;
  std::list<float> m_FrameTimeWindow;
  bool m_PrintFrameRate;
  bool m_PrintMinFrameRate;

  bool onKeyPressEvent(Engine::KeyPressEvent& event);
};