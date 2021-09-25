#pragma once
#include "Engine.h"
#include "World/Chunk.h"

class GameSandbox : public Engine::Layer
{
public:
  GameSandbox();
  ~GameSandbox() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(std::chrono::duration<float> timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  Engine::CameraController m_CameraController;
  Engine::Shared<Engine::Texture2D> m_CheckerboardTexture;

  Chunk m_Chunk;
};