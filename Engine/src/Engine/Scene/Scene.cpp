#include "ENpch.h"
#include "Scene.h"
#include "Components.h"
#include "DevCamera.h"
#include "Engine/Renderer/Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  struct ECS
  {
    static entt::registry& Registry() { return Entity::Registry(); }
  };

  static constexpr Vec3 s_UpDirection(0, 0, 1);
  
  Entity Scene::CreateEntity(const std::string& name)
  {
    return CreateEntity(Vec3(0.0), name);
  }

  Entity Scene::CreateEntity(const Vec3& initialPosition, const std::string& name)
  {
    Entity entity(ECS::Registry().create());
    entity.add<Component::Tag>().name = name.empty() ? "Unnamed Entity" : name;
    entity.add<Component::Transform>().position = initialPosition;
    return entity;
  }

  Entity Scene::CreateEmptyEntity()
  {
    return ECS::Registry().create();
  }

  void Scene::DestroyEntity(Entity entity)
  {
    ECS::Registry().destroy(entity);
  }

  Entity Scene::GetEntity(uint32_t entityID)
  {
    if (ECS::Registry().valid(static_cast<entt::entity>(entityID)))
      return Entity(static_cast<entt::entity>(entityID));
    else
    {
      EN_ERROR("Entity ID does not refer to a valid entity!");
      return {};
    }
  }

  void Scene::OnUpdate(Timestep timestep)
  {
    // Update scripts
    ECS::Registry().view<Component::NativeScript>().each([=](entt::entity entityID, Component::NativeScript& nsc)
      {
        // TODO: Move to Scene::OnScenePlay
        if (!nsc.instance)
        {
          nsc.instance = nsc.instantiateScript();
          nsc.instance->entity = Entity(entityID);
          nsc.instance->onCreate();
        }

        nsc.instance->onUpdate(timestep);
      });

    // Render 2D
    auto view = ECS::Registry().view<Component::SpriteRenderer>();
    if (view.size() > 0)
    {
      Mat4 viewProj = ActiveCameraViewProjection();

      Renderer2D::BeginScene(viewProj);
      for (entt::entity entityID : view)
      {
        Mat4 transform = ECS::Registry().get<Component::Transform>(entityID).calculateTransform();
        const Component::SpriteRenderer& sprite = view.get<Component::SpriteRenderer>(entityID);

        Renderer2D::DrawSprite(transform, sprite, static_cast<int>(entityID));
      }
      Renderer2D::EndScene();
    }
  }

  void Scene::OnUpdateDev(Timestep timestep)
  {
    // Render 2D
    auto view = ECS::Registry().view<Component::SpriteRenderer>();
    if (view.size() > 0)
    {
      Renderer2D::BeginScene(DevCamera::ViewProjection());
      for (entt::entity entityID : view)
      {
        Mat4 transform = ECS::Registry().get<Component::Transform>(entityID).calculateTransform();
        const Component::SpriteRenderer& sprite = view.get<Component::SpriteRenderer>(entityID);

        Renderer2D::DrawSprite(transform, sprite, static_cast<int>(entityID));
      }
      Renderer2D::EndScene();
    }
  }

  void Scene::OnEvent(Event& event)
  {
    ECS::Registry().view<Component::NativeScript>().each([&](entt::entity entityID, Component::NativeScript& nsc)
      {
        // TODO: Move to Scene::OnScenePlay
        if (!nsc.instance)
        {
          nsc.instance = nsc.instantiateScript();
          nsc.instance->entity = Entity(entityID);
          nsc.instance->onCreate();
        }

        nsc.instance->onEvent(event);
      });
  }

  Mat4 Scene::ActiveCameraViewProjection()
  {
    auto view = ECS::Registry().view<Component::Camera>();
    for (entt::entity entityID : view)
    {
      const Component::Camera& cameraComponent = view.get<Component::Camera>(entityID);

      if (cameraComponent.isActive)
      {
        const Mat4& projection = view.get<Component::Camera>(entityID).camera.getProjection();
        
        Mat4 viewMatrix{};
        Entity entity(entityID);
        if (cameraComponent.camera.getProjectionType() == Camera::ProjectionType::Perspective)
        {
          Vec3 viewDirection = entity.get<Component::Transform>().orientationDirection();
          const Vec3& position = entity.get<Component::Transform>().position;
          viewMatrix = glm::lookAt(position, position + viewDirection, s_UpDirection);
        }
        else if (cameraComponent.camera.getProjectionType() == Camera::ProjectionType::Orthographic)
          viewMatrix = glm::inverse(entity.get<Component::Transform>().calculateTransform());
        else
          EN_CORE_ERROR("Unknown camera projection type!");

        return projection * viewMatrix;
      }
    }

    EN_ERROR("No active camera found!");
    return {};
  }

  void Scene::OnViewportResize(uint32_t width, uint32_t height)
  {
    // Resize our non-fixed aspect ratio cameras
    auto view = ECS::Registry().view<Component::Camera>();
    for (entt::entity entityID : view)
    {
      Component::Camera& cameraComponent = view.get<Component::Camera>(entityID);
      if (!cameraComponent.fixedAspectRatio)
        cameraComponent.camera.setViewportSize(width, height);
    }
  }

  void Scene::ForEachEntity(void (*func)(const Entity))
  {
    const size_t numEntities = ECS::Registry().size();
    const entt::entity* entityIDs = ECS::Registry().data();

    for (int i = 0; i < numEntities; ++i)
      func(Entity(entityIDs[i]));
  }
}