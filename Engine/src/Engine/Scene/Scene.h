#pragma once
#include "Entity.h"
#include "Engine/Core/Time.h"
#include "Engine/Events/Event.h"
#include "Engine/Math/Vec.h"

// TODO: Turn into class
namespace eng::scene
{
  Entity CreateEntity(std::string_view name = {});
  Entity CreateEntity(const math::Vec3& initialPosition, std::string_view name = {});
  Entity CreateEmptyEntity();
  void DestroyEntity(Entity entity);
  Entity GetEntity(u32 entityID);

  void OnUpdate(Timestep timestep);
  void OnEvent(event::Event& event);

  Entity ActiveCamera();
  math::Mat4 CalculateViewProjection(Entity viewer);

  void OnViewportResize(u32 width, u32 height);

  void ForEachEntity(void (*func)(const Entity));
}