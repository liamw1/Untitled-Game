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
    Float4 color = Float4(1.0);
  };

  struct CircleRenderer
  {
    Float4 color = Float4(1.0);
    float thickness = 1.0f;
    float fade = 0.005f;
  };

  struct Camera
  {
    Engine::Camera camera;

    bool isActive = false;
    bool fixedAspectRatio = false;
  };

  struct NativeScript
  {
    std::unique_ptr<Engine::EntityScript> instance;
    std::unique_ptr<Engine::EntityScript> (*instantiateScript)(Engine::Entity entity);

    template<typename T, typename... Args>
    void bind(Args... args)
    {
      instantiateScript = [args...](Engine::Entity entity)->std::unique_ptr<Engine::EntityScript> { return std::make_unique<T>(entity, args...); };
    }
  };
}