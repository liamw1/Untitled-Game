#include "GDpch.h"
#include "EditorLayer.h"
#include "Panels/SceneHierarchyPanel.h"

#pragma warning( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#pragma warning( pop )

namespace Engine
{
  EditorLayer::EditorLayer()
    : Layer("EditorLayer"),
      m_ViewportSize({0, 0})
  {
    RenderCommand::Initialize();
    Renderer2D::Initialize();
  }

  void EditorLayer::onAttach()
  {
    EN_PROFILE_FUNCTION();

    m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");

    FramebufferSpecification framebufferSpecification;
    framebufferSpecification.width = 1280;
    framebufferSpecification.height = 720;
    m_Framebuffer = Framebuffer::Create(framebufferSpecification);

    m_SquareEntity = Scene::CreateEntity("Square");
    m_SquareEntity.add<Component::SpriteRenderer>(Vec4(0.0, 1.0, 0.0, 1.0));

    m_CameraEntity = Scene::CreateEntity("Primary Camera");
    m_CameraEntity.add<Component::Camera>().isActive = true;
    m_CameraEntity.get<Component::Camera>().camera.setOrthographic(1.0f, 3.0f, 0.0f, 1.0f);

    m_SecondCamera = Scene::CreateEntity("Secondary Camera");
    m_SecondCamera.add<Component::Camera>().camera.setOrthographic(1.0f, 3.0f, 0.0f, 1.0f);

    class CameraController : public ScriptableEntity
    {
    public:
      void onCreate()
      {
        Mat4& transform = get<Component::Transform>().transform;
        transform[3][0] = static_cast<length_t>(rand() % 30) / 10 - static_cast<length_t>(1.0);
      }

      void onUpdate(Timestep timestep)
      {
        seconds dt = timestep.sec();
        length_t speed = 5.0;

        Mat4& transform = get<Component::Transform>();

        if (Input::IsKeyPressed(Key::A))
          transform[3][0] -= speed * dt;
        if (Input::IsKeyPressed(Key::D))
          transform[3][0] += speed * dt;
        if (Input::IsKeyPressed(Key::W))
          transform[3][1] += speed * dt;
        if (Input::IsKeyPressed(Key::S))
          transform[3][1] -= speed * dt;
      }
    };

    m_CameraEntity.add<Component::NativeScript>().bind<CameraController>();
    m_SecondCamera.add<Component::NativeScript>().bind<CameraController>();
  }

  void EditorLayer::onDetach()
  {
  }

  void EditorLayer::onUpdate(Timestep timestep)
  {
    EN_PROFILE_FUNCTION();

    // Resize
    FramebufferSpecification spec = m_Framebuffer->getSpecification();
    if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && (spec.width != m_ViewportSize.x || spec.height != m_ViewportSize.y))
    {
      uint32_t viewportWidth = static_cast<uint32_t>(m_ViewportSize.x);
      uint32_t viewportHeight = static_cast<uint32_t>(m_ViewportSize.y);

      m_Framebuffer->resize(viewportWidth, viewportHeight);
      Scene::OnViewportResize(viewportWidth, viewportHeight);
    }

    // Render
    Renderer2D::ResetStats();
    m_Framebuffer->bind();
    RenderCommand::Clear(Vec4(0.1f, 0.1f, 0.1f, 1.0f));

    // Update scene
    Scene::OnUpdate(timestep);

    m_Framebuffer->unbind();
  }

  void EditorLayer::onImGuiRender()
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

        if (ImGui::MenuItem("Exit")) Application::Get().close();
        ImGui::EndMenu();
      }

      ImGui::EndMenuBar();
    }

    SceneHierarchyPanel::OnImGuiRender();

    ImGui::Begin("Statistics");

    auto stats = Renderer2D::GetStats();
    ImGui::Text("Renderer2D Stats:");
    ImGui::Text("Draw Calls: %d", stats.drawCalls);
    ImGui::Text("Quads: %d", stats.quadCount);
    ImGui::Text("Vertices: %d", stats.getTotalVertexCount());
    ImGui::Text("Indices: %d", stats.getTotatlIndexCount());

    {
      ImGui::Separator();
      const std::string& name = m_SquareEntity.get<Component::Tag>().name;
      ImGui::Text("%s", name.c_str());

      Float4& squareColor = m_SquareEntity.get<Component::SpriteRenderer>().color;
      ImGui::ColorEdit4("Square Color", glm::value_ptr(squareColor));
      ImGui::Separator();
    }

    ImGui::DragFloat3("Camera Transform", glm::value_ptr(m_CameraEntity.get<Component::Transform>().transform[3]));

    bool primaryCamera = m_CameraEntity.get<Component::Camera>().isActive;
    if (ImGui::Checkbox("Camera A", &primaryCamera))
    {
      m_CameraEntity.get<Component::Camera>().isActive = primaryCamera;
      m_SecondCamera.get<Component::Camera>().isActive = !primaryCamera;
    }

    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsWindowHovered();
    if (m_ViewportFocused)
      Application::Get().getImGuiLayer()->blockEvents(!m_ViewportFocused || !m_ViewportHovered);

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    m_ViewportSize = Float2(viewportPanelSize.x, viewportPanelSize.y );

    uintptr_t textureID = m_Framebuffer->getColorAttachmentRendererID();
    ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::End();
  }

  void EditorLayer::onEvent(Event& event)
  {
  }
}