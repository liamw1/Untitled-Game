#include "Sandbox2D.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D()
  : Layer("Sandbox2D"),
    m_CameraController(1280.0f / 720.0f, true)
{
}

void Sandbox2D::onAttach()
{
}

void Sandbox2D::onDetach()
{
}

void Sandbox2D::onUpdate(std::chrono::duration<float> timestep)
{
  const float dt = timestep.count();  // Time between frames in seconds
    // EN_TRACE("dt: {0}ms", dt * 1000.0f);

  m_CameraController.onUpdate(timestep);

  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  Engine::Renderer2D::BeginScene(m_CameraController.getCamera());
  Engine::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.8f, 0.2f, 0.3f, 1.0f });
  Engine::Renderer2D::EndScene();
}

void Sandbox2D::onImGuiRender()
{
  ImGui::Begin("Settings");
  ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
  ImGui::End();
}

void Sandbox2D::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}
