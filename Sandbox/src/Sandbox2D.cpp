#include "Sandbox2D.h"
#include <imgui.h>

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
  m_SpriteSheet = Engine::Texture2D::Create("assets/textures/voxel-pack/Spritesheets/spritesheet_tiles.png");

  m_RockTexture = Engine::SubTexture2D::CreateFromIndices(m_SpriteSheet, 128, 3, 1);
  m_MossyRockTexture = Engine::SubTexture2D::CreateFromIndices(m_SpriteSheet, 128, 3, 2);
}

void Sandbox2D::onDetach()
{
  EN_PROFILE_FUNCTION();
}

void Sandbox2D::onUpdate(std::chrono::duration<float> timestep)
{
  EN_PROFILE_FUNCTION();

  Engine::Renderer2D::ResetStats();
  
  m_CameraController.onUpdate(timestep);

  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  static radians rotation = 0.0f;
  rotation += timestep.count();

#if 0
  Engine::Renderer2D::BeginScene(m_CameraController.getCamera());
  Engine::Renderer2D::DrawQuad({ {0.0f, 0.0f, -0.1f}, glm::vec2(50.f), glm::vec4(1.0f), 10.0f}, m_CheckerboardTexture);
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      Engine::Renderer2D::DrawRotatedQuad({ { (float)i - 2.0f, (float)j - 2.0f, 0.0f }, { 0.66f, 0.66f }, {0.8f, 0.2f, 0.3f, 1.0f}}, rotation, m_CheckerboardTexture);

  for (float y = -5.0f; y < 5.0f; y += 0.5f)
    for (float x = -5.0f; x < 5.0f; x += 0.5f)
    {
      glm::vec4 color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.5f };
      Engine::Renderer2D::DrawQuad({ { x, y, 0.0f }, glm::vec2(0.45f), color, 1.0f });
    }
  Engine::Renderer2D::EndScene();
#endif

  Engine::Renderer2D::BeginScene(m_CameraController.getCamera());
  Engine::Renderer2D::DrawQuad({ {0.0f, 0.0f, 0.0f}, glm::vec2(1.0f), glm::vec4(1.0f), 1.0f }, m_RockTexture);
  Engine::Renderer2D::DrawQuad({ {1.0f, 1.0f, 0.0f}, glm::vec2(1.0f), glm::vec4(1.0f), 1.0f }, m_MossyRockTexture);
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
