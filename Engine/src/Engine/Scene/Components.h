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

    Vec3 getPosition() const { return Vec3(transform[3]); }
    void setPosition(const Vec3& newPosition) { transform[3] = Vec4(newPosition, transform[3][3]); }
  };

  struct Orientation
  {
    Angle roll = Angle(0.0f);     // Rotation around x-axis
    Angle pitch = Angle(0.0f);    // Rotation around y-axis
    Angle yaw = Angle(-90.0f);    // Rotation around z-axis

    // Converts from rotation angle coordinates to Cartesian coordinates
    Vec3 orientationDirection() const
    {
      return { cos(yaw.rad()) * cos(pitch.rad()),
              -sin(yaw.rad()) * cos(pitch.rad()),
              -sin(pitch.rad()) };
    }
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