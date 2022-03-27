#pragma once

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
    Unique<Framebuffer> m_Framebuffer;

    Entity m_GreenSquareEntity;
    Entity m_RedSquareEntity;
    Entity m_CameraEntity;
    Entity m_SecondCamera;
    Entity m_HoveredEntity;

    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;
    Float2 m_ViewportSize;
    std::array<Float2, 2> m_ViewportBounds{};

    bool onMouseButtonPress(MouseButtonPressEvent& event);
  };
}