#pragma once

class Sandbox2D : public Engine::Layer
{
public:
  Sandbox2D();
  ~Sandbox2D() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(Timestep timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  Unique<Engine::Texture2D> m_CheckerboardTexture;

  Engine::Entity m_CameraEntity;
};