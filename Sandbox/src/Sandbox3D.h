#pragma once
#include "Engine.h"

class Sandbox3D : public Engine::Layer
{
public:
  Sandbox3D();
  ~Sandbox3D() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(std::chrono::duration<float> timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  Engine::CameraController m_CameraController;
  Engine::Shared<Engine::Texture2D> m_CheckerboardTexture;
};