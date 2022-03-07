#include "Sandbox2D.h"

#include <codeanalysis\warnings.h> // Disable intellisense warnings
#pragma warning( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#include <imgui.h>
#pragma warning( pop )

Sandbox2D::Sandbox2D()
  : Layer("Sandbox2D"),
    m_CameraController(1280.0f / 720.0f, true)
{
  Engine::RenderCommand::Initialize();
  Engine::Renderer2D::Initialize();
}

void Sandbox2D::onAttach()
{
  EN_PROFILE_FUNCTION();

  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");

  m_CameraEntity = Engine::Scene::CreateEntity();
  m_CameraEntity.add<Engine::Component::Camera>(m_CameraController.getViewProjectionMatrix(), true);
}

void Sandbox2D::onDetach()
{
}

void Sandbox2D::onUpdate(Engine::Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  // Update
  m_CameraController.onUpdate(timestep);
  m_CameraEntity.get<Engine::Component::Camera>().viewProjection = m_CameraController.getViewProjectionMatrix();

  // Render
  Engine::Renderer2D::ResetStats();
  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  static radians rotation = 0.0;
  rotation += timestep.sec();

  Engine::Renderer2D::BeginScene(Engine::Scene::GetActiveCamera().viewProjection);
  Engine::Renderer2D::DrawQuad({ {0.0, 0.0, -0.1}, Vec2(50.0), Float4(1.0f), 10.0f}, m_CheckerboardTexture);
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      Engine::Renderer2D::DrawRotatedQuad({ { static_cast<length_t>(i) - 2.0, static_cast<length_t>(j) - 2.0, 0.0 }, { 0.66, 0.66 }, {0.8f, 0.2f, 0.3f, 1.0f}}, rotation, m_CheckerboardTexture);

  for (length_t y = -5.0; y < 5.0; y += 0.5)
    for (length_t x = -5.0; x < 5.0; x += 0.5)
    {
      Float4 color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.5f };
      Engine::Renderer2D::DrawQuad({ { x, y, 0.0 }, Vec2(static_cast<length_t>(0.45)), color, 1.0f });
    }

  Engine::Renderer2D::EndScene();
}

void Sandbox2D::onImGuiRender()
{
  EN_PROFILE_FUNCTION();

  ImGui::Begin("Settings");

  auto stats = Engine::Renderer2D::GetStats();
  ImGui::Text("Renderer2D Stats:");
  ImGui::Text("Draw Calls: %d", stats.drawCalls);
  ImGui::Text("Quads: %d", stats.quadCount);
  ImGui::Text("Vertices: %d", stats.getTotalVertexCount());
  ImGui::Text("Indices: %d", stats.getTotatlIndexCount());

  ImGui::End();
}

void Sandbox2D::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}
