#include "ENpch.h"
#include "Scene.h"
#include "Components.h"
#include "Engine/Renderer/Renderer2D.h"
#include "Engine/Renderer/DevCamera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  static constexpr Vec3 s_UpDirection(0, 0, 1);
  static entt::registry s_Registry;
  
  Entity Scene::CreateEntity(const std::string& name)
  {
    return CreateEntity(Vec3(0.0), name);
  }

  Entity Scene::CreateEntity(const Vec3& initialPosition, const std::string& name)
  {
    Entity entity(s_Registry, s_Registry.create());
    entity.add<Component::Tag>().name = name.empty() ? "Unnamed Entity" : name;
    entity.add<Component::Transform>().position = initialPosition;
    return entity;
  }

  void Scene::DestroyEntity(Entity entity)
  {
    s_Registry.destroy(entity);
  }

  void Scene::OnUpdate(Timestep timestep)
  {
    // Update scripts
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

    // Render 2D
    auto view = s_Registry.view<Component::SpriteRenderer>();
    if (view.size() > 0)
    {
      Mat4 viewProj = ActiveCameraViewProjection();

      Renderer2D::BeginScene(viewProj);
      for (entt::entity entityID : view)
      {
        Mat4 transform = s_Registry.get<Component::Transform>(entityID).calculateTransform();
        const Float4& color = view.get<Component::SpriteRenderer>(entityID).color;

        Renderer2D::DrawQuad(transform, color);
      }
      Renderer2D::EndScene();
    }
  }

  void Scene::OnUpdateDev(Timestep timestep)
  {
    // Render 2D
    auto view = s_Registry.view<Component::SpriteRenderer>();
    if (view.size() > 0)
    {
      Renderer2D::BeginScene(DevCamera::ViewProjection());
      for (entt::entity entityID : view)
      {
        Mat4 transform = s_Registry.get<Component::Transform>(entityID).calculateTransform();
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