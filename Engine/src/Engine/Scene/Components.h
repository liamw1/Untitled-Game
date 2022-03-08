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
    Mat4 transform = Mat4(1.0);

    operator Mat4& () { return transform; }
    operator const Mat4& () { return transform; }
  };

  struct Orientation
  {
    Angle roll = Angle(0.0f);     // Rotation around x-axis
    Angle pitch = Angle(0.0f);    // Rotation around y-axis
    Angle yaw = Angle(-90.0f);    // Rotation around z-axis
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