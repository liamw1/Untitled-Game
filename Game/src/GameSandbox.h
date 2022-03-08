#pragma once
#include "World/World.h"

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
  Shared<Engine::Texture2D> m_CheckerboardTexture;
  World m_World;

  bool onKeyPressEvent(Engine::KeyPressEvent& event);
};