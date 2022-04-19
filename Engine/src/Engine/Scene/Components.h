#pragma once
#include "Scripting.h"
#include "Engine/Core/UID.h"
#include "Engine/Renderer/Camera.h"

// Forward declare entity
namespace Engine
{
  class Entity;
}

/*
  Generic entity components
*/
namespace Component
{
  struct ID
  {
    Engine::UID ID;
  };

  struct Tag
  {
    std::string name{};
  };

  struct Transform
  {
    Vec3 position{};
    Vec3 rotation{};          // Rotation around x,y,z axis
    Vec3 scale = Vec3(1.0);

    Vec3 orientationDirection() const;
    Mat4 calculateTransform() const;
  };

  struct SpriteRenderer
  {
    Vec4 color = Vec4(1.0);
  };

  struct Camera
  {
    Engine::Camera camera;

    bool isActive = false;
    bool fixedAspectRatio = false;
  };

  struct NativeScript
  {
    Unique<Engine::EntityScript> instance;
    Unique<Engine::EntityScript> (*instantiateScript)(Engine::Entity entity);

    template<typename T, typename... Args>
    void bind(Args... args)
    {
      instantiateScript = [args...](Engine::Entity entity)->Unique<Engine::EntityScript> { return CreateUnique<T>(entity, args...); };
    }
  };
}