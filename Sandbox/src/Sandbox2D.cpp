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

  Engine::FramebufferSpecification framebufferSpecification;
  framebufferSpecification.width = 1280;
  framebufferSpecification.height = 720;
  m_Framebuffer = Engine::Framebuffer::Create(framebufferSpecification);
}

void Sandbox2D::onDetach()
{
}

void Sandbox2D::onUpdate(std::chrono::duration<seconds> timestep)
{
  EN_PROFILE_FUNCTION();

  // Update
  m_CameraController.onUpdate(timestep);

  // Render
  Engine::Renderer2D::ResetStats();
  m_Framebuffer->bind();
  Engine::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1.0f });

  static radians rotation = 0.0;
  rotation += timestep.count();

  Engine::Renderer2D::BeginScene(m_CameraController.getCamera());
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
  m_Framebuffer->unbind();
}

void Sandbox2D::onImGuiRender()
{
  EN_PROFILE_FUNCTION();

  static bool dockspaceOpen = true;
  static bool opt_fullscreen = true;
  static bool opt_padding = false;
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

  // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
  // because it would be confusing to have two docking targets within each others.
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  if (opt_fullscreen)
  {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  }
  else
  {
    dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
  }

  // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
  // and handle the pass-thru hole, so we ask Begin() to not render a background.
  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
// all active windows docked into it will lose their parent and become undocked.
// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
  if (!opt_padding)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
  if (!opt_padding)
    ImGui::PopStyleVar();

  if (opt_fullscreen)
    ImGui::PopStyleVar(2);

// DockSpace
  ImGuiIO& io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
  {
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
  }

  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("Options"))
    {
      // Disabling fullscreen would allow the window to be moved to the front of other windows,
      // which we can't undo at the moment without finer window depth/z control.
      ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
      ImGui::MenuItem("Padding", NULL, &opt_padding);
      ImGui::Separator();

      if (ImGui::MenuItem("Exit")) Engine::Application::Get().close();
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  ImGui::Begin("Settings");

  auto stats = Engine::Renderer2D::GetStats();
  ImGui::Text("Renderer2D Stats:");
  ImGui::Text("Draw Calls: %d", stats.drawCalls);
  ImGui::Text("Quads: %d", stats.quadCount);
  ImGui::Text("Vertices: %d", stats.getTotalVertexCount());
  ImGui::Text("Indices: %d", stats.getTotatlIndexCount());

  uintptr_t textureID = m_Framebuffer->getColorAttachmentRendererID();
  ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ 1280, 720 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
  ImGui::End();

  ImGui::End();
}

void Sandbox2D::onEvent(Engine::Event& event)
{
  m_CameraController.onEvent(event);
}
