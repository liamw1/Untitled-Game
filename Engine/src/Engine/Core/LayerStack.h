#pragma once
#include "Layer.h"

namespace eng
{
  class LayerStack
  {
  public:
    using iterator = std::list<std::unique_ptr<Layer>>::iterator;
    using const_iterator = std::list<std::unique_ptr<Layer>>::const_iterator;
    using reverse_iterator = std::list<std::unique_ptr<Layer>>::reverse_iterator;
    using const_reverse_iterator = std::list<std::unique_ptr<Layer>>::const_reverse_iterator;

    LayerStack();
    ~LayerStack();

    void pushLayer(std::unique_ptr<Layer> layer);
    void popLayer(iterator layerPosition);
    void popLayer(reverse_iterator layerPosition);

    iterator begin();
    iterator end();
    reverse_iterator rbegin();
    reverse_iterator rend();

    const_iterator begin() const;
    const_iterator end() const;
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;

  private:
    std::list<std::unique_ptr<Layer>> m_Layers;
  };
}