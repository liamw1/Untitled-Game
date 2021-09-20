#pragma once

/*
  Abstract representation of a graphics context.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  class GraphicsContext
  {
  public:
    virtual void initialize() = 0;
    virtual void swapBuffers() = 0;
  };
}