#pragma once
#include "ENpch.h"

namespace Engine
{
  class GraphicsContext
  {
  public:
    virtual void initialize() = 0;
    virtual void swapBuffers() = 0;
  };
}