#pragma once
#include "Entity.h"
#include "Engine/Renderer/Camera.h"

/*
  Generic entity components
*/
namespace Component
{
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
    Engine::ScriptableEntity* instance = nullptr;
      
    Engine::ScriptableEntity* (*instantiateScript)();
    void (*destroyScript)(NativeScript*);

    template<typename T>
    void bind()
    {
      instantiateScript = []() { return static_cast<Engine::ScriptableEntity*>(new T()); };
      destroyScript = [](NativeScript* nsc) { delete nsc->instance; nsc->instance = nullptr; };
    }
  };
}