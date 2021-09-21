#include "Sandbox2D.h"
#include <imgui.h>

Sandbox2D::Sandbox2D()
  : Layer("Sandbox2D"),
    m_CameraController(1280.0f / 720.0f, true)
{
}

void Sandbox2D::onAttach()
{
  EN_PROFILE_FUNCTION();

  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");
}

void Sandbox2D::onDetach()
{
  EN_PROFILE_FUNCTION();
}

void Sandbox2D::onUpdate(std::chrono::duration<float> timestep)
{
  EN_PROFILE_FUNCTION();
  
  m_CameraController.onUpdate(timestep);

  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  Engine::Renderer2D::BeginScene(m_CameraController.getCamera());
  Engine::Renderer2D::DrawQuad({ { -1.0f, 1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f }, 1.0f });
  Engine::Renderer2D::DrawQuad({ { 0.5, -0.5f, -0.0f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f }, 1.0f});
  Engine::Renderer2D::EndScene();
}

void Sandbox2D::onImGuiRender()
{
  EN_PROFILE_FUNCTION();

  ImGui::Begin("Settings");
  // ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
  ImGui::End();
}

void Sandbox2D::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}
