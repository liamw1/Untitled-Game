#pragma once
#include "Engine.h"

namespace Engine
{
  class EditorLayer : public Layer
  {
  public:
    EditorLayer();
    ~EditorLayer() = default;

    void onAttach() override;
    void onDetach() override;

    void onUpdate(std::chrono::duration<seconds> timestep) override;
    void onImGuiRender() override;
    void onEvent(Event& event) override;

  private:
    OrthographicCameraController m_CameraController;
    Shared<Texture2D> m_CheckerboardTexture;

    Shared<Framebuffer> m_Framebuffer;

    Float2 m_ViewportSize{};
  };
}