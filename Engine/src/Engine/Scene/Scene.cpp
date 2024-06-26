#include "ENpch.h"
#include "Scene.h"
#include "Components.h"
#include "DevCamera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace eng
{
  class ECS
  {
  public:
    static Entity GetEntity(entt::entity entityID)
    {
      ENG_CORE_ASSERT(Entity::Registry().valid(entityID), "Entity ID does not refer to a valid entity!");
      return Entity(entityID);
    }
    static Entity GetEntity(u32 entityID) { return static_cast<entt::entity>(entityID); }

    static entt::registry& Registry() { return Entity::Registry(); }
    static Entity Create() { return Entity::Registry().create(); }
  };
}

namespace eng::scene
{
  static constexpr math::Vec3 c_UpDirection(0, 0, 1);
  
  Entity CreateEntity(std::string_view name)
  {
    return CreateEntity(math::Vec3(0.0), name);
  }

  Entity CreateEntity(const math::Vec3& initialPosition, std::string_view name)
  {
    Entity entity = ECS::Create();
    entity.add<component::ID>();
    entity.add<component::Tag>().name = name.empty() ? "Unnamed Entity" : name;
    entity.add<component::Transform>().position = initialPosition;
    return entity;
  }

  Entity CreateEmptyEntity()
  {
    return ECS::Create();
  }

  void DestroyEntity(Entity entity)
  {
    ECS::Registry().destroy(entity);
  }

  Entity GetEntity(u32 entityID)
  {
    return ECS::GetEntity(entityID);
  }

  void OnUpdate(Timestep timestep)
  {
    // Update scripts
    ECS::Registry().view<component::NativeScript>().each([=](entt::entity entityID, component::NativeScript& nsc)
      {
        // TODO: Move to scene::onScenePlay
        if (!nsc.instance)
          nsc.instance = nsc.instantiateScript(ECS::GetEntity(entityID));

        nsc.instance->onUpdate(timestep);
      });
  }

  void OnEvent(event::Event& event)
  {
    ECS::Registry().view<component::NativeScript>().each([&](entt::entity entityID, component::NativeScript& nsc)
      {
        // TODO: Move to scene::onScenePlay
        if (!nsc.instance)
          nsc.instance = nsc.instantiateScript(ECS::GetEntity(entityID));

        nsc.instance->onEvent(event);
      });
  }

  Entity ActiveCamera()
  {
    auto view = ECS::Registry().view<component::Camera>();
    for (entt::entity entityID : view)
    {
      const component::Camera& cameraComponent = view.get<component::Camera>(entityID);

      if (cameraComponent.isActive)
        return ECS::GetEntity(entityID);
    }

    ENG_CORE_ERROR("No active camera found!");
    return {};
  }

  math::Mat4 CalculateViewProjection(Entity viewer)
  {
    const Camera& camera = viewer.get<component::Camera>().camera;
    const math::Mat4& projection = camera.projectionMatrix();

    math::Mat4 viewMatrix{};
    if (camera.projectionType() == Camera::ProjectionType::Perspective)
    {
      math::Vec3 viewDirection = viewer.get<component::Transform>().orientationDirection();
      const math::Vec3& position = viewer.get<component::Transform>().position;
      viewMatrix = glm::lookAt(position, position + viewDirection, c_UpDirection);
    }
    else if (camera.projectionType() == Camera::ProjectionType::Orthographic)
      viewMatrix = glm::inverse(viewer.get<component::Transform>().calculateTransform());
    else
      ENG_CORE_ERROR("Unknown camera projection type!");

    return projection * viewMatrix;
  }

  void OnViewportResize(u32 width, u32 height)
  {
    // Resize our non-fixed aspect ratio cameras
    auto view = ECS::Registry().view<component::Camera>();
    for (entt::entity entityID : view)
    {
      component::Camera& cameraComponent = view.get<component::Camera>(entityID);
      if (!cameraComponent.fixedAspectRatio)
        cameraComponent.camera.setViewportSize(width, height);
    }
  }

  void ForEachEntity(void (*func)(const Entity))
  {
    const uSize numEntities = ECS::Registry().size();
    const entt::entity* entityIDs = ECS::Registry().data();

    for (i32 i = 0; i < numEntities; ++i)
      func(ECS::GetEntity(entityIDs[i]));
  }
}