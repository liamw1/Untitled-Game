#pragma once
#include <Engine.h>

namespace Engine
{
  class EditorLayer : public Layer
  {
  public:
    EditorLayer();
    ~EditorLayer() = default;

    void onAttach() override;
    void onDetach() override;

    void onUpdate(Timestep timestep) override;
    void onImGuiRender() override;
    void onEvent(Event& event) override;

  private:
    enum class SceneState
    {
      Edit = 0,
      Play
    };

    SceneState m_SceneState = SceneState::Edit;

    Unique<Framebuffer> m_Framebuffer;

    Entity m_GreenSquareEntity;
    Entity m_RedSquareEntity;
    Entity m_CameraEntity;
    Entity m_SecondCamera;
    Entity m_HoveredEntity;

    Unique<Texture2D> m_CheckerboardTexture;
    Unique<Texture2D> m_PlayButtonIcon;
    Unique<Texture2D> m_StopButtonIcon;

    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;
    Float2 m_ViewportSize;
    std::array<Float2, 2> m_ViewportBounds{};

    void onScenePlay();
    void onSceneStop();

    void UI_Toolbar();

    bool onMouseButtonPress(MouseButtonPressEvent& event);
  };
}