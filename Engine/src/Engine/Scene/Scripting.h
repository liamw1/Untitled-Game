#pragma once
#include "Engine/Core/Time.h"
#include "Engine/Events/Event.h"

namespace Engine
{
  class EntityScript
  {
  public:
    virtual ~EntityScript() = default;

    virtual void onUpdate(Timestep timestep) = 0;
    virtual void onEvent(Event& event) = 0;
  };
}