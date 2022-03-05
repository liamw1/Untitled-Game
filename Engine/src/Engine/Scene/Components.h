#pragma once

namespace Engine
{
  namespace Component
  {
    struct Position
    {
      Vec3 position{};
    };

    struct Transform
    {
      Mat4 transform = Mat4(1.0);
    };

    struct SpriteRenderer
    {
      Vec4 color = Vec4(1.0);
    };

    struct OrthographicCamera
    {
      Engine::OrthographicCamera camera;
    };
  }
}