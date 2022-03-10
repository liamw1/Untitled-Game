#include "ENpch.h"
#include "Scene.h"
#include "Components.h"
#include "Engine/Renderer/Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  static constexpr Vec3 s_UpDirection(0, 0, 1);
  static entt::registry s_Registry;
  
  Entity Scene::CreateEntity(const std::string& name)
  {
    Entity entity(s_Registry, s_Registry.create());
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
    Camera* mainCamera = nullptr;
    Mat4 viewProj{};
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
      auto view = s_Registry.view<Component::Transform, Component::SpriteRenderer>();
      for (entt::entity entityID : view)
      {
        const Mat4& transform = view.get<Component::Transform>(entityID).transform;
        const Float4& color = view.get<Component::SpriteRenderer>(entityID).color;

        Renderer2D::DrawQuad(transform, color);
      }
      Renderer2D::EndScene();
    }
  }

  void Scene::OnEvent(Event& event)
  {
    s_Registry.view<Component::NativeScript>().each([&](entt::entity entityID, Component::NativeScript& nsc)
      {
        // TODO: Move to Scene::OnScenePlay
        if (!nsc.instance)
        {
          nsc.instance = nsc.instantiateScript();
          nsc.instance->entity = Entity(s_Registry, entityID);
          nsc.instance->onCreate();
        }

        nsc.instance->onEvent(event);
      });
  }

  Mat4 Scene::ActiveCameraViewProjection()
  {
    auto view = s_Registry.view<Component::Camera>();
    for (entt::entity entityID : view)
    {
      const Component::Camera& cameraComponent = view.get<Component::Camera>(entityID);

      if (cameraComponent.isActive)
      {
        const Mat4& projection = view.get<Component::Camera>(entityID).camera.getProjection();
        
        Mat4 viewMatrix{};
        Entity entity(s_Registry, entityID);
        if (entity.has<Component::Orientation>())
        {
          Vec3 viewDirection = entity.get<Component::Orientation>().orientationDirection();
          Vec3 position = entity.get<Component::Transform>().transform[3];
          viewMatrix = glm::lookAt(position, position + viewDirection, s_UpDirection);
        }
        else if (entity.has<Component::Transform>())
          viewMatrix = entity.get<Component::Transform>();
        else
          EN_WARN("Active camera has no transform!");

        return projection * viewMatrix;
      }
    }

    EN_ERROR("No active camera found!");
    return {};
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