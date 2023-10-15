#pragma once
#include "Scripting.h"
#include "Engine/Core/UID.h"
#include "Engine/Renderer/Camera.h"

// Forward declare entity
namespace eng
{
  class Entity;
}

/*
  Generic entity components
*/
namespace eng::component
{
  struct ID
  {
    UID ID;
  };

  struct Tag
  {
    std::string name{};
  };

  struct Transform
  {
    math::Vec3 position{};
    math::Vec3 rotation{};          // Rotation around x,y,z axis
    math::Vec3 scale = math::Vec3(1.0);

    math::Vec3 orientationDirection() const;
    math::Mat4 calculateTransform() const;
  };

  struct SpriteRenderer
  {
    math::Float4 color = math::Float4(1.0);
  };

  struct CircleRenderer
  {
    math::Float4 color = math::Float4(1.0);
    float thickness = 1.0f;
    float fade = 0.005f;
  };

  struct Camera
  {
    eng::Camera camera;

    bool isActive = false;
    bool fixedAspectRatio = false;
  };

  struct NativeScript
  {
    std::unique_ptr<EntityScript> instance;
    std::unique_ptr<EntityScript> (*instantiateScript)(Entity entity);

    template<typename T, typename... Args>
    void bind(Args... args)
    {
      instantiateScript = [args...](Entity entity)->std::unique_ptr<EntityScript> { return std::make_unique<T>(entity, args...); };
    }
  };
}