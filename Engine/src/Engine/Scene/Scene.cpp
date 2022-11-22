#include "ENpch.h"
#include "Scene.h"
#include "Components.h"
#include "DevCamera.h"
#include "Engine/Renderer/Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
  class ECS
  {
  public:
    static Entity GetEntity(entt::entity entityID)
    {
      EN_CORE_ASSERT(Entity::Registry().valid(entityID), "Entity ID does not refer to a valid entity!");
      return Entity(entityID);
    }
    static Entity GetEntity(uint32_t entityID) { return (static_cast<entt::entity>(entityID)); }

    static entt::registry& Registry() { return Entity::Registry(); }
    static Entity Create() { return Entity::Registry().create(); }
  };

  static constexpr Vec3 s_UpDirection(0, 0, 1);
  
  Entity Scene::CreateEntity(const std::string& name)
  {
    return CreateEntity(Vec3(0.0), name);
  }

  Entity Scene::CreateEntity(const Vec3& initialPosition, const std::string& name)
  {
    Entity entity = ECS::Create();
    entity.add<Component::ID>();
    entity.add<Component::Tag>().name = name.empty() ? "Unnamed Entity" : name;
    entity.add<Component::Transform>().position = initialPosition;
    return entity;
  }

  Entity Scene::CreateEmptyEntity()
  {
    return ECS::Create();
  }

  void Scene::DestroyEntity(Entity entity)
  {
    ECS::Registry().destroy(entity);
  }

  Entity Scene::GetEntity(uint32_t entityID)
  {
    return ECS::GetEntity(entityID);
  }

  static void render2D(const Mat4& viewProjection)
  {
    auto spriteView = ECS::Registry().view<Component::SpriteRenderer>();
    auto circleView = ECS::Registry().view<Component::CircleRenderer>();

    if (spriteView.size() == 0 && circleView.size() == 0)
      return;

    Renderer2D::BeginScene(viewProjection);

    for (entt::entity entityID : spriteView)
    {
      Mat4 transform = ECS::Registry().get<Component::Transform>(entityID).calculateTransform();
      const Component::SpriteRenderer& sprite = spriteView.get<Component::SpriteRenderer>(entityID);

      Renderer2D::DrawSprite(transform, sprite, static_cast<int>(entityID));
    }

    for (entt::entity entityID : circleView)
    {
      Mat4 transform = ECS::Registry().get<Component::Transform>(entityID).calculateTransform();
      const Component::CircleRenderer& circle = circleView.get<Component::CircleRenderer>(entityID);

      Renderer2D::DrawCircle(transform, circle.color, circle.thickness, circle.fade, static_cast<int>(entityID));
    }

    Renderer2D::EndScene();
  }

  void Scene::OnUpdate(Timestep timestep)
  {
    // Update scripts
    ECS::Registry().view<Component::NativeScript>().each([=](entt::entity entityID, Component::NativeScript& nsc)
      {
        // TODO: Move to Scene::OnScenePlay
        if (!nsc.instance)
          nsc.instance = nsc.instantiateScript(ECS::GetEntity(entityID));

        nsc.instance->onUpdate(timestep);
      });

    render2D(ActiveCameraViewProjection());
  }

  void Scene::OnUpdateDev(Timestep timestep)
  {
    render2D(DevCamera::ViewProjection());
  }

  void Scene::OnEvent(Event& event)
  {
    ECS::Registry().view<Component::NativeScript>().each([&](entt::entity entityID, Component::NativeScript& nsc)
      {
        // TODO: Move to Scene::OnScenePlay
        if (!nsc.instance)
          nsc.instance = nsc.instantiateScript(ECS::GetEntity(entityID));

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
        Entity entity = ECS::GetEntity(entityID);
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
      func(ECS::GetEntity(entityIDs[i]));
  }
}