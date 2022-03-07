#include "ENpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Engine/Renderer/Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  static entt::registry s_Registry;
  
  Entity Scene::CreateEntity()
  {
    return Entity(s_Registry, s_Registry.create());
  }

  void Scene::OnUpdate(Timestep timestep)
  {
    // Update scripts
    {
      s_Registry.view<Component::NativeScript>().each([=](auto entity, auto& nsc)
        {
          // TODO: Move to Scene::OnScenePlay
          if (!nsc.instance)
          {
            nsc.instance = nsc.instantiateScript();
            nsc.instance->entity = Entity(s_Registry, entity);
            nsc.instance->onCreate();
          }

          nsc.instance->onUpdate(timestep);
        });
    }

    // Find active camera
    SceneCamera* mainCamera = nullptr;
    Mat4 viewProj = Mat4{};
    {
      auto view = s_Registry.view<Component::Camera>();
      for (auto entity : view)
      {
        Component::Camera& cameraComponent = view.get<Component::Camera>(entity);
        if (cameraComponent.isActive)
        {
          mainCamera = &cameraComponent.camera;

          viewProj = mainCamera->getProjection();
          if (s_Registry.any_of<Component::Transform>(entity))
            viewProj *= glm::inverse(s_Registry.get<Component::Transform>(entity).transform);

          break;
        }
      }
    }

    // Render 2D
    if (mainCamera)
    {
      Renderer2D::BeginScene(viewProj);
      auto group = s_Registry.group<Component::Transform>(entt::get<Component::SpriteRenderer>);
      for (auto entity : group)
      {
        const auto [transform, sprite] = group.get<Component::Transform, Component::SpriteRenderer>(entity);

        Renderer2D::DrawQuad(transform.transform, sprite.color);
      }
      Renderer2D::EndScene();
    }
  }

  const SceneCamera& Scene::GetActiveCamera()
  {
    auto view = s_Registry.view<Component::Camera>();
    for (auto entity : view)
    {
      const Component::Camera& cameraComponent = view.get<Component::Camera>(entity);
      
      if (cameraComponent.isActive)
        return cameraComponent.camera;
    }

    EN_ERROR("No active camera found!");
    return view.get<Component::Camera>(entt::null).camera;
  }

  void Scene::OnViewportResize(uint32_t width, uint32_t height)
  {
    // Resize our non-fixed aspect ratio cameras
    auto view = s_Registry.view<Component::Camera>();
    for (auto entity : view)
    {
      Component::Camera& cameraComponent = view.get<Component::Camera>(entity);
      if (!cameraComponent.fixedAspectRatio)
        cameraComponent.camera.setViewportSize(width, height);
    }
  }
}