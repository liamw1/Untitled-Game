#include "ENpch.h"
#include "LayerStack.h"
#include "Engine/Debug/Instrumentor.h"

namespace eng
{
  LayerStack::LayerStack()
    : m_Layers(), m_LayerInsertIndex(0) {}

  LayerStack::~LayerStack()
  {
    ENG_PROFILE_FUNCTION();

    for (Layer* layer : m_Layers)
    {
      layer->onDetach();
      delete layer;
    }
  }

  void LayerStack::pushLayer(Layer* layer)
  {
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
    m_LayerInsertIndex++;
  }

  void LayerStack::pushOverlay(Layer* overlay)
  {
    m_Layers.push_back(overlay);
  }

  void LayerStack::popLayer(Layer* layer)
  {
    ENG_PROFILE_FUNCTION();

    auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
    if (it != m_Layers.begin() + m_LayerInsertIndex)
    {
      layer->onDetach();
      m_Layers.erase(it);
      m_LayerInsertIndex--;
    }
  }

  void LayerStack::popOverlay(Layer* overlay)
  {
    ENG_PROFILE_FUNCTION();

    auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
    if (it != m_Layers.end())
    {
      overlay->onDetach();
      m_Layers.erase(it);
    }
  }

  std::vector<Layer*>::iterator LayerStack::begin() { return m_Layers.begin(); }
  std::vector<Layer*>::iterator LayerStack::end() { return m_Layers.end(); }
  std::vector<Layer*>::reverse_iterator LayerStack::rbegin() { return m_Layers.rbegin(); }
  std::vector<Layer*>::reverse_iterator LayerStack::rend() { return m_Layers.rend(); }

  std::vector<Layer*>::const_iterator LayerStack::begin() const { return m_Layers.begin(); }
  std::vector<Layer*>::const_iterator LayerStack::end() const { return m_Layers.end(); }
  std::vector<Layer*>::const_reverse_iterator LayerStack::rbegin() const { return m_Layers.rbegin(); }
  std::vector<Layer*>::const_reverse_iterator LayerStack::rend() const { return m_Layers.rend(); }
}