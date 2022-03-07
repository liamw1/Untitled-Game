#pragma once
#include "Engine.h"

class Sandbox2D : public Engine::Layer
{
public:
  Sandbox2D();
  ~Sandbox2D() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(Engine::Timestep timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  Engine::OrthographicCameraController m_CameraController;
  Engine::Shared<Engine::Texture2D> m_CheckerboardTexture;

  Engine::Entity m_CameraEntity;
};