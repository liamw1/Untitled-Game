#pragma once
#include "Engine/Events/Event.h"

namespace Engine
{
  class Layer
  {
  public:
    Layer(const std::string& debugName = "Layer");
    virtual ~Layer();

    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(Timestep /*timestep*/) {}
    virtual void onImGuiRender() {}
    virtual void onEvent(Event& /*event*/) {}

    const std::string& getName() const { return m_DebugName; }

  protected:
    std::string m_DebugName;
  };
}