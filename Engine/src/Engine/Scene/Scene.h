#pragma once
#include "Engine/Renderer/SceneCamera.h"

namespace Engine
{
  class Entity;

  namespace Scene
  {
    Entity CreateEntity();

    void OnUpdate(Timestep timestep);

    const SceneCamera& GetActiveCamera();

    void OnViewportResize(uint32_t width, uint32_t height);
  };
}