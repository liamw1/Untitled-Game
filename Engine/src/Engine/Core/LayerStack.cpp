#include "ENpch.h"
#include "LayerStack.h"
#include "Engine/Debug/Instrumentor.h"

namespace eng
{
  LayerStack::LayerStack() = default;

  LayerStack::~LayerStack()
  {
    ENG_PROFILE_FUNCTION();

    for (std::unique_ptr<Layer>& layer : m_Layers)
      layer->onDetach();
  }

  void LayerStack::pushLayer(std::unique_ptr<Layer> layer)
  {
    ENG_PROFILE_FUNCTION();

    layer->onAttach();
    m_Layers.push_back(std::move(layer));
  }

  void LayerStack::popLayer(iterator layerPosition)
  {
    ENG_PROFILE_FUNCTION();

    (*layerPosition)->onDetach();
    m_Layers.erase(layerPosition);
  }

  void LayerStack::popLayer(reverse_iterator layerPosition)
  {
    popLayer(std::next(layerPosition).base());
  }

  LayerStack::iterator LayerStack::begin() { return m_Layers.begin(); }
  LayerStack::iterator LayerStack::end() { return m_Layers.end(); }
  LayerStack::reverse_iterator LayerStack::rbegin() { return m_Layers.rbegin(); }
  LayerStack::reverse_iterator LayerStack::rend() { return m_Layers.rend(); }

  LayerStack::const_iterator LayerStack::begin() const { return m_Layers.begin(); }
  LayerStack::const_iterator LayerStack::end() const { return m_Layers.end(); }
  LayerStack::const_reverse_iterator LayerStack::rbegin() const { return m_Layers.rbegin(); }
  LayerStack::const_reverse_iterator LayerStack::rend() const { return m_Layers.rend(); }
}