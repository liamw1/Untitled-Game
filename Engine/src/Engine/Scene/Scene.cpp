#include "ENpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Engine/Renderer/Renderer2D.h"

namespace Engine
{
  static entt::registry s_Registry;
  
  Entity Scene::CreateEntity()
  {
    return Entity(s_Registry, s_Registry.create());
  }

  void Scene::OnUpdate(std::chrono::duration<seconds> timestep)
  {
    auto group = s_Registry.group<Component::Position>(entt::get<Component::SpriteRenderer>);
    for (auto entity : group)
    {
      const auto& [position, sprite] = group.get<Component::Position, Component::SpriteRenderer>(entity);
      
      Renderer2D::DrawQuad({ position.position, Vec2(1.0), sprite.color, 1.0f });
    }
  }
}