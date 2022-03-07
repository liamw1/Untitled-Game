#include "GDpch.h"
#include "SceneHierarchyPanel.h"


#pragma warning( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#include <imgui.h>
#pragma warning( pop )

namespace Engine
{
  static Entity s_SelectionContext;

  static void drawEntityNode(Entity entity)
  {
    const std::string& name = entity.get<Component::Tag>().name;
    const uintptr_t entityID = entity.id();

    ImGuiTreeNodeFlags flags = ((s_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(entityID), flags, name.c_str());
    if (ImGui::IsItemClicked())
      s_SelectionContext = entity;

    if (opened)
    {
      ImGui::TreePop();
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

    ImGui::End();
  }
}