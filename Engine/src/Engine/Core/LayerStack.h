#pragma once
#include "Layer.h"

namespace eng
{
  class LayerStack
  {
    std::list<std::unique_ptr<Layer>> m_Layers;

  public:
    LayerStack();
    ~LayerStack();

    ENG_DEFINE_ITERATORS(m_Layers);

    void pushLayer(std::unique_ptr<Layer> layer);
    void popLayer(iterator layerPosition);
    void popLayer(reverse_iterator layerPosition);
  };
}