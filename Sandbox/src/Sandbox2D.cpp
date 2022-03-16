#include "SBpch.h"
#include "Sandbox2D.h"

Sandbox2D::Sandbox2D()
  : Layer("Sandbox2D")
{
  Engine::RenderCommand::Initialize();
  Engine::Renderer2D::Initialize();
}

void Sandbox2D::onAttach()
{
  EN_PROFILE_FUNCTION();

  m_CheckerboardTexture = Engine::Texture2D::Create("assets/textures/Checkerboard.png");

  m_CameraEntity = Engine::Scene::CreateEntity();
  m_CameraEntity.add<Component::Camera>().isActive = true;
  m_CameraEntity.get<Component::Camera>().camera.setOrthographic(1.0f, 10.0f, 0.0f, 1.0f);
}

void Sandbox2D::onDetach()
{
}

void Sandbox2D::onUpdate(Timestep timestep)
{
  EN_PROFILE_FUNCTION();

  // Render
  Engine::Renderer2D::ResetStats();
  Engine::RenderCommand::Clear(Float4(0.1f, 0.1f, 0.1f, 1.0f));

  static Angle rotation(0.0f);
  rotation += Angle::FromRad(timestep.sec());

  Engine::Renderer2D::BeginScene(Engine::Scene::ActiveCameraViewProjection());
  Engine::Renderer2D::DrawQuad(Vec3(0.0, 0.0, -0.1), Vec2(50.0), Float4(1.0f), 10.0f, m_CheckerboardTexture);
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      Engine::Renderer2D::DrawQuad(Vec3(i - 2, j - 2, 0), Vec2(0.66), Float4(0.8f, 0.2f, 0.3f, 1.0f), 1.0f, m_CheckerboardTexture, rotation);
  
  for (float y = -5.0; y < 5.0; y += 0.5)
    for (float x = -5.0; x < 5.0; x += 0.5)
    {
      Float4 color((x + 5.0f) / 10, 0.4f, (y + 5.0f) / 10, 0.5f);
      Engine::Renderer2D::DrawQuad(Vec3(x, y, 0), Vec2(0.45), color);
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
}
