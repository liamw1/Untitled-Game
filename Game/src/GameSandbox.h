#pragma once
#include "World/World.h"

class GameSandbox : public Engine::Layer
{
public:
  GameSandbox();
  ~GameSandbox() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(std::chrono::duration<seconds> timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  Engine::Shared<Engine::Texture2D> m_CheckerboardTexture;
  World m_World;
  Player m_Player;

  bool onKeyPressEvent(Engine::KeyPressEvent& event);
};