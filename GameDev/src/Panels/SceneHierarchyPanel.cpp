#include "GDpch.h"
#include "SceneHierarchyPanel.h"
#include <imgui_internal.h>

namespace Engine
{
  static Entity s_SelectionContext;
  static char s_TextBuffer[256];

  static void drawEntityNode(const Entity& entity)
  {
    const std::string& name = entity.get<Component::Tag>().name;
    const uintptr_t entityID = entity.id();

    ImGuiTreeNodeFlags flags = ((s_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(entityID), flags, name.c_str());
    if (ImGui::IsItemClicked())
      s_SelectionContext = entity;

    if (opened)
      ImGui::TreePop();
  }

  static void drawVec3Control(const std::string& label, Vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
  {
    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    float lineHeight = GImGui->Font->FontSize + 2.0f * GImGui->Style.FramePadding.y;
    ImVec2 buttonSize(lineHeight + 3.0f, lineHeight);

    static constexpr char* buttonLabels[3] = { "X", "Y", "Z" };
    static constexpr char* dragFloatLabels[3] = { "##X", "##Y", "##Z" };
    static const ImVec4 buttonColors[3] = { ImVec4(0.9f, 0.2f, 0.2f, 1.0f), 
                                            ImVec4(0.3f, 0.8f, 0.3f, 1.0f), 
                                            ImVec4(0.2f, 0.35f, 0.9f, 1.0f) };
    for (int i = 0; i < 3; ++i)
    {
      static constexpr float dim = 0.85f;
      ImVec4 inactiveButtonColor = ImVec4(dim * buttonColors[i].x, dim * buttonColors[i].y, dim * buttonColors[i].z, 1.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, inactiveButtonColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColors[i]);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, inactiveButtonColor);
      if (ImGui::Button(buttonLabels[i], buttonSize))
        values[i] = resetValue;
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImGui::DragFloat(dragFloatLabels[i], &values[i], 0.1f, 0.0f, 0.0f, "%.2f");
      ImGui::PopItemWidth();
      ImGui::SameLine();
    }

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
  }

  static void drawComponents(Entity& entity)
  {
    if (entity.has<Component::Tag>())
    {
      std::string& name = entity.get<Component::Tag>().name;

      memset(s_TextBuffer, 0, sizeof(s_TextBuffer));
      strcpy_s(s_TextBuffer, sizeof(s_TextBuffer), name.c_str());
      if (ImGui::InputText("Tag", s_TextBuffer, sizeof(s_TextBuffer)))
        name = std::string(s_TextBuffer);
    }

    if (entity.has<Component::Transform>())
      if (ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(Component::Transform).hash_code()), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
      {
        Component::Transform& transformComponent = entity.get<Component::Transform>();

        drawVec3Control("Position", transformComponent.position);
        Vec3 rotation = glm::degrees(transformComponent.rotation);
        drawVec3Control("Rotation", rotation);
        transformComponent.rotation = glm::radians(rotation);
        drawVec3Control("Scale", transformComponent.scale, 1.0f);

        ImGui::TreePop();
      }

    if (entity.has<Component::Camera>())
      if (ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(Component::Camera).hash_code()), ImGuiTreeNodeFlags_DefaultOpen, "Camera"))
      {
        Camera& camera = entity.get<Component::Camera>().camera;

        static constexpr char* projectionTypeStrings[2] = { "Perspective", "Orthographic" };
        const char* currentProjectionTypeString = projectionTypeStrings[static_cast<int>(camera.getProjectionType())];
        if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
        {
          for (int i = 0; i < 2; ++i)
            if (currentProjectionTypeString == projectionTypeStrings[i])
              ImGui::SetItemDefaultFocus();

          ImGui::EndCombo();
        }

        float aspectRatio = camera.getAspectRatio();
        float nearClip = camera.getNearClip();
        float farClip = camera.getFarClip();
        if (camera.getProjectionType() == Camera::ProjectionType::Perspective)
        {
          float fov = camera.getFOV().deg();
          ImGui::DragFloat("FOV", &fov);
          ImGui::DragFloat("Near", &nearClip);
          ImGui::DragFloat("Far", &farClip);

          camera.setPerspective(aspectRatio, Angle(fov), nearClip, farClip);
        }
        else if (camera.getProjectionType() == Camera::ProjectionType::Orthographic)
        {
          float orthoSize = camera.getOrthographicSize();

          ImGui::DragFloat("Size", &orthoSize);
          ImGui::DragFloat("Near", &nearClip);
          ImGui::DragFloat("Far", &farClip);

          camera.setOrthographic(aspectRatio, orthoSize, nearClip, farClip);
        }
        else
          EN_CORE_ERROR("Unknown camera projection type!");

        ImGui::TreePop();
      }

    if (entity.has<Component::SpriteRenderer>())
      if (ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(Component::SpriteRenderer).hash_code()), ImGuiTreeNodeFlags_DefaultOpen, "Sprite Renderer"))
      {
        Component::SpriteRenderer& spriteRendererComponent = entity.get<Component::SpriteRenderer>();
        ImGui::ColorEdit4("Color", glm::value_ptr(spriteRendererComponent.color));
        ImGui::TreePop();
      }
  }

  void SceneHierarchyPanel::OnImGuiRender()
  {
    ImGui::Begin("Scene Hierarchy");

    Scene::Registry().each([](entt::entity entityID)
      {
        Entity entity(Scene::Registry(), entityID);
        drawEntityNode(entity);
      });

    // Deselect panel selection
    if (ImGui::IsMouseDown(static_cast<mouseCode>(Mouse::Button0)) && ImGui::IsWindowHovered())
      s_SelectionContext = {};

    ImGui::End();

    ImGui::Begin("Properties");
    if (s_SelectionContext.isValid())
      drawComponents(s_SelectionContext);
    ImGui::End();
  }
}