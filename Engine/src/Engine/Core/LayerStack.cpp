#include "ENpch.h"
#include "LayerStack.h"

namespace Engine
{
  LayerStack::~LayerStack()
  {
    EN_PROFILE_FUNCTION();

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
    m_Layers.emplace_back(overlay);
  }

  void LayerStack::popLayer(Layer* layer)
  {
    EN_PROFILE_FUNCTION();

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
    EN_PROFILE_FUNCTION();

    auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
    if (it != m_Layers.end())
    {
      overlay->onDetach();
      m_Layers.erase(it);
    }
  }
}