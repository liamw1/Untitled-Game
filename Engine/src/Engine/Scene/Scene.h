#pragma once
#include "entt/entt.hpp"

namespace Engine
{
  class Entity;

  namespace Scene
  {
    Entity CreateEntity();

    void OnUpdate(std::chrono::duration<seconds> timestep);
  };
}