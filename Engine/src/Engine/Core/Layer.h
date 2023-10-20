#pragma once
#include "Time.h"
#include "Engine/Events/Event.h"

namespace eng
{
  class Layer
  {
  public:
    Layer(const std::string& debugName = "Layer")
    : m_DebugName(debugName) {}
    virtual ~Layer() = default;

    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(Timestep /*timestep*/) {}
    virtual void onEvent(event::Event& /*event*/) {}

    const std::string& getName() const { return m_DebugName; }

  protected:
    std::string m_DebugName;
  };
}