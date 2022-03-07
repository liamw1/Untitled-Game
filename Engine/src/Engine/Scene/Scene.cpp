#include "ENpch.h"
#include "Scene.h"
#include "Components.h"
#include "Engine/Renderer/Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  static entt::registry s_Registry;
  
  Entity Scene::CreateEntity(const std::string& name)
  {
    Entity entity = Entity(s_Registry, s_Registry.create());
    entity.add<Component::Transform>();
    entity.add<Component::Tag>().name = name.empty() ? "Unnamed Entity" : name;
    return entity;
  }

  void Scene::OnUpdate(Timestep timestep)
  {
    // Update scripts
    {
      s_Registry.view<Component::NativeScript>().each([=](entt::entity entityID, Component::NativeScript& nsc)
        {
          // TODO: Move to Scene::OnScenePlay
          if (!nsc.instance)
          {
            nsc.instance = nsc.instantiateScript();
            nsc.instance->entity = Entity(s_Registry, entityID);
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
      for (entt::entity entityID : view)
      {
        Component::Camera& cameraComponent = view.get<Component::Camera>(entityID);
        if (cameraComponent.isActive)
        {
          mainCamera = &cameraComponent.camera;

          viewProj = mainCamera->getProjection();
          if (s_Registry.any_of<Component::Transform>(entityID))
            viewProj *= glm::inverse(s_Registry.get<Component::Transform>(entityID).transform);

          break;
        }
      }
    }

    // Render 2D
    if (mainCamera)
    {
      Renderer2D::BeginScene(viewProj);
      auto group = s_Registry.group<Component::Transform>(entt::get<Component::SpriteRenderer>);
      for (entt::entity entityID : group)
      {
        const auto [transform, sprite] = group.get<Component::Transform, Component::SpriteRenderer>(entityID);

        Renderer2D::DrawQuad(transform.transform, sprite.color);
      }
      Renderer2D::EndScene();
    }
  }

  const SceneCamera& Scene::GetActiveCamera()
  {
    auto view = s_Registry.view<Component::Camera>();
    for (entt::entity entityID : view)
    {
      const Component::Camera& cameraComponent = view.get<Component::Camera>(entityID);
      
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
    for (entt::entity entityID : view)
    {
      Component::Camera& cameraComponent = view.get<Component::Camera>(entityID);
      if (!cameraComponent.fixedAspectRatio)
        cameraComponent.camera.setViewportSize(width, height);
    }
  }

  entt::registry& Scene::Registry()
  {
    return s_Registry;
  }
}