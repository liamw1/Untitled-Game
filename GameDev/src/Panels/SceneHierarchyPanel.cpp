#include "GDpch.h"
#include "SceneHierarchyPanel.h"
#include <imgui_internal.h>

namespace Engine
{
  static Entity s_SelectionContext;
  static char s_TextBuffer[256];
  static constexpr ImGuiTreeNodeFlags s_TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen |
                                                        ImGuiTreeNodeFlags_Framed |
                                                        ImGuiTreeNodeFlags_SpanAvailWidth |
                                                        ImGuiTreeNodeFlags_AllowItemOverlap |
                                                        ImGuiTreeNodeFlags_FramePadding;

  static void drawEntityNode(const Entity& entity)
  {
    const std::string& name = entity.get<Component::Tag>().name;
    const uintptr_t entityID = entity.id();

    ImGuiTreeNodeFlags flags = (s_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0;
    flags |= ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(entityID), flags, name.c_str());
    if (ImGui::IsItemClicked())
      s_SelectionContext = entity;

    if (ImGui::BeginPopupContextItem())
    {
      if (ImGui::MenuItem("Delete Entity"))
        Scene::DestroyEntity(entity);

      ImGui::EndPopup();
    }

    if (opened)
      ImGui::TreePop();
  }

  template<typename T, typename UIFunction>
  static void drawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
  {
    if (entity.has<T>())
    {
      T& component = entity.get<T>();
      float contentRegionAvailalbeWidth = ImGui::GetContentRegionAvailWidth();

      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
      float lineHeight = GImGui->Font->FontSize + 2.0f * GImGui->Style.FramePadding.y;
      ImGui::Separator();
      bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(T).hash_code()), s_TreeNodeFlags, name.c_str());
      ImGui::PopStyleVar();

      ImGui::SameLine(contentRegionAvailalbeWidth - lineHeight / 2);
      if (ImGui::Button("...", ImVec2(lineHeight, lineHeight)))
        ImGui::OpenPopup("ComponentSettings");

      bool componentMarkedForRemoval = false;
      if (ImGui::BeginPopup("ComponentSettings"))
      {
        if (ImGui::MenuItem("Remove Component"))
          componentMarkedForRemoval = true;

        ImGui::EndPopup();
      }

      if (open)
      {
        uiFunction(component);
        ImGui::TreePop();
      }

      if (componentMarkedForRemoval)
        s_SelectionContext.remove<T>();
    }
  }

  static void drawVec3Control(const std::string& label, Vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
  {
    ImGuiIO& io = ImGui::GetIO();
    ImFont* boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    float lineHeight = GImGui->Font->FontSize + 2.0f * GImGui->Style.FramePadding.y;
    ImVec2 buttonSize(lineHeight + 3.0f, lineHeight);

    static const std::string buttonLabels[3] = { "X", "Y", "Z" };
    static const ImVec4 buttonColors[3] = { ImVec4(0.9f, 0.2f, 0.2f, 1.0f), 
                                            ImVec4(0.3f, 0.8f, 0.3f, 1.0f), 
                                            ImVec4(0.2f, 0.35f, 0.9f, 1.0f) };
    for (int i = 0; i < 3; ++i)
    {
      static constexpr float dim = 0.85f;
      ImVec4 dimButtonColor = ImVec4(dim * buttonColors[i].x, dim * buttonColors[i].y, dim * buttonColors[i].z, 1.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, dimButtonColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColors[i]);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, dimButtonColor);

      ImGui::PushFont(boldFont);
      if (ImGui::Button(buttonLabels[i].c_str(), buttonSize))
        values[i] = resetValue;
      ImGui::PopFont();
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      std::string dragFloatLabel = "##" + buttonLabels[i];
      ImGui::DragFloat(dragFloatLabel.c_str(), &values[i], 0.1f, 0.0f, 0.0f, "%.2f");
      ImGui::PopItemWidth();
      if (i < 2)
        ImGui::SameLine();
    }

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
  }

  template<typename T>
  static void addComponentEntry(const std::string& entryName)
  {
    if (ImGui::MenuItem(entryName.c_str()))
    {
      if (!s_SelectionContext.has<T>())
      {
        s_SelectionContext.add<T>();
        ImGui::CloseCurrentPopup();
      }
      else
        EN_CORE_WARN("Entity already has {0} component", entryName);
    }
  }

  static void drawComponents(Entity& entity)
  {
    if (entity.has<Component::Tag>())
    {
      std::string& name = entity.get<Component::Tag>().name;

      memset(s_TextBuffer, 0, sizeof(s_TextBuffer));
      strcpy_s(s_TextBuffer, sizeof(s_TextBuffer), name.c_str());
      if (ImGui::InputText("##Tag", s_TextBuffer, sizeof(s_TextBuffer)))
        name = std::string(s_TextBuffer);
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(-1);

    if (ImGui::Button("Add Component"))
      ImGui::OpenPopup("AddComponent");

    if (ImGui::BeginPopup("AddComponent"))
    {
      addComponentEntry<Component::Camera>("Camera");
      addComponentEntry<Component::SpriteRenderer>("Sprite Renderer");
      addComponentEntry<Component::CircleRenderer>("Circle Renderer");

      ImGui::EndPopup();
    }
    ImGui::PopItemWidth();

    drawComponent<Component::ID>("ID", entity, [](auto& component)
      {
        ImGui::Text("ID: %s", component.ID.toString().c_str());
      });

    if (entity.has<Component::Transform>())
      if (ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(Component::Transform).hash_code()), s_TreeNodeFlags, "Transform"))
      {
        Component::Transform& transformComponent = entity.get<Component::Transform>();

        drawVec3Control("Position", transformComponent.position);
        Vec3 rotation = glm::degrees(transformComponent.rotation);
        drawVec3Control("Rotation", rotation);
        transformComponent.rotation = glm::radians(rotation);
        drawVec3Control("Scale", transformComponent.scale, 1.0f);

        ImGui::TreePop();
      }

    drawComponent<Component::Camera>("Camera", entity, [](auto& component)
      {
        Camera& camera = component.camera;

        static constexpr std::array<const char*, 2> projectionTypeStrings = { "Perspective", "Orthographic" };
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

          camera.setPerspective(aspectRatio, Angle::FromDeg(fov), nearClip, farClip);
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
      });

    drawComponent<Component::SpriteRenderer>("Sprite renderer", entity, [](auto& component)
      {
        ImGui::ColorEdit4("Color", glm::value_ptr(component.color));
      });

    drawComponent<Component::CircleRenderer>("Circle renderer", entity, [](auto& component)
      {
        ImGui::ColorEdit4("Color", glm::value_ptr(component.color));
        ImGui::DragFloat("Thickness", &component.thickness, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat("Fade", &component.fade, 0.001f, 0.0f, 1.0f);
      });
  }

  void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
  {
    s_SelectionContext = entity;
  }

  void SceneHierarchyPanel::OnImGuiRender()
  {
    ImGui::Begin("Scene Hierarchy");

    Scene::ForEachEntity([](const Entity entity) { drawEntityNode(entity); });

    // Deselect panel selection
    if (ImGui::IsMouseDown(static_cast<mouseCode>(Mouse::ButtonLeft)) && ImGui::IsWindowHovered())
      s_SelectionContext = {};

    // Right-click on blank space
    if (ImGui::BeginPopupContextWindow(0, 1, false))
    {
      if (ImGui::MenuItem("Create Empty Entity"))
        Scene::CreateEntity("Empty Entity");

      ImGui::EndPopup();
    }

    ImGui::End();

    ImGui::Begin("Properties");
    if (s_SelectionContext.isValid())
    {
      drawComponents(s_SelectionContext);
    }
    ImGui::End();
  }
}