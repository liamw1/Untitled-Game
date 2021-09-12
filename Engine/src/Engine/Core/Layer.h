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
    virtual void onUpdate(std::chrono::duration<float> timestep) {}
    virtual void onImGuiRender() {}
    virtual void onEvent(Event& event) {}

    inline const std::string& getName() const { return m_DebugName; }

  protected:
    std::string m_DebugName;
  };
}