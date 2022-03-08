#pragma once

class Sandbox3D : public Engine::Layer
{
public:
  Sandbox3D();
  ~Sandbox3D() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(Timestep timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  Shared<Engine::Texture2D> m_CheckerboardTexture;

  Engine::Entity m_CameraEntity;
};