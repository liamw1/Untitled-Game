#include "GDpch.h"
#include "SceneHierarchyPanel.h"

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

  static void drawComponents(Entity& entity)
  {
    if (entity.has<Component::Transform>())
    {
      std::string& name = entity.get<Component::Tag>().name;

      memset(s_TextBuffer, 0, sizeof(s_TextBuffer));
      strcpy_s(s_TextBuffer, sizeof(s_TextBuffer), name.c_str());
      if (ImGui::InputText("Tag", s_TextBuffer, sizeof(s_TextBuffer))) 
        name = std::string(s_TextBuffer);
    }

    if (entity.has<Component::Transform>())
    {
      if (ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(Component::Transform).hash_code()), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
      {
        Mat4& transform = entity.get<Component::Transform>().transform;
        ImGui::DragFloat3("Position", glm::value_ptr(transform[3]), 0.1f);

        ImGui::TreePop();
      }
    }
  }

  void SceneHierarchyPanel::OnImGuiRender()
  {
    ImGui::Begin("Scene Hierarchy");

    Scene::Registry().each([](entt::entity entityID)
      {
        Entity entity = Entity(Scene::Registry(), entityID);
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