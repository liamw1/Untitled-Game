#pragma once
#include "Time.h"
#include "Engine/Events/Event.h"

namespace eng
{
  class Layer
  {
  protected:
    std::string m_DebugName;

  public:
    Layer(std::string_view debugName = "Layer")
    : m_DebugName(debugName) {}
    virtual ~Layer() = default;

    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(Timestep /*timestep*/) {}
    virtual void onEvent(event::Event& /*event*/) {}

    std::string_view getName() const { return m_DebugName; }
  };
}