#pragma once
#include "Entity.h"
#include "Engine/Renderer/SceneCamera.h"

namespace Engine
{
  namespace Component
  {
    struct Transform
    {
      Mat4 transform = Mat4(1.0);

      operator Mat4& () { return transform; }
      operator const Mat4& () { return transform; }
    };

    struct SpriteRenderer
    {
      Vec4 color = Vec4(1.0);
    };

    struct Camera
    {
      SceneCamera camera;

      bool isActive = false;
      bool fixedAspectRatio = false;
    };

    struct NativeScript
    {
      ScriptableEntity* instance = nullptr;
        
      ScriptableEntity* (*instantiateScript)();
      void (*destroyScript)(NativeScript*);

      template<typename T>
      void bind()
      {
        instantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
        destroyScript = [](NativeScript* nsc) { delete nsc->instance; nsc->instance = nullptr; };
      }
    };
  }
}