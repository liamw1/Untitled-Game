#pragma once
#include "ENpch.h"
#include "Engine/Events/Event.h"

namespace Engine
{
  class ENGINE_API Layer
  {
  public:
    Layer(const std::string& debugName = "Layer");
    virtual ~Layer();

    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate() {}
    virtual void onEvent(Event& event) {}

    inline const std::string& getName() const { return m_DebugName; }

  protected:
    std::string m_DebugName;
  };
}