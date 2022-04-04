#pragma once
#include "Entity.h"

namespace Engine
{
  namespace Scene
  {
    Entity CreateEntity(const std::string& name = std::string());
    Entity CreateEntity(const Vec3& initialPosition, const std::string& name = std::string());
    Entity CreateEmptyEntity();
    void DestroyEntity(Entity entity);
    Entity GetEntity(uint32_t entityID);

    void OnUpdate(Timestep timestep);
    void OnUpdateDev(Timestep timestep);
    void OnEvent(Event& event);

    Mat4 ActiveCameraViewProjection();

    void OnViewportResize(uint32_t width, uint32_t height);

    void ForEachEntity(void (*func)(const Entity));
  };
}