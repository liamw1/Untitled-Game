#pragma once

namespace Engine
{
  class Event;

  class Layer
  {
  public:
    Layer(const std::string& debugName = "Layer")
    : m_DebugName(debugName) {}
    virtual ~Layer() = default;

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