#pragma once
#include "Engine.h"

class Sandbox2D : public Engine::Layer
{
public:
  Sandbox2D();
  ~Sandbox2D() = default;

  void onAttach() override;
  void onDetach() override;

  void onUpdate(std::chrono::duration<uint64_t, std::nano> timestep) override;
  void onImGuiRender() override;
  void onEvent(Engine::Event& event) override;

private:
  struct ProfileResult
  {
    const char* name;
    float time;
  };
  std::vector<ProfileResult> m_ProfileResults;

  Engine::OrthographicCameraController m_CameraController;

  // TEMPORARY
  Engine::Shared<Engine::VertexArray> m_SquareVA;
  Engine::Shared<Engine::Shader> m_FlatColorShader;

  Engine::Shared<Engine::Texture2D> m_CheckerboardTexture;

  glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
};