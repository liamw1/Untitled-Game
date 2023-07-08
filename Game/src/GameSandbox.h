#pragma once
#include "World/World.h"
#include <Engine.h>

class GameSandbox : public Engine::Layer
{
public:
  GameSandbox();
  ~GameSandbox() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(Timestep timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  World m_World;
  bool m_PrintFrameRate;

  bool onKeyPressEvent(Engine::KeyPressEvent& event);
};