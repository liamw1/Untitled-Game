#pragma once
#include "Engine/Events/Event.h"

namespace Engine
{
  class EntityScript
  {
  public:
    virtual ~EntityScript() {}

    virtual void onUpdate(Timestep timestep) {};
    virtual void onEvent(Event& event) {};
  };
}