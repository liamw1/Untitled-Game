#pragma once
#include "Buffer.h"

/*
  Abstract representation of a vertex array.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  class VertexArray
  {
  public:
    virtual ~VertexArray() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual void addVertexBuffer(const Shared<VertexBuffer>& vertexBuffer) = 0;

    /*
      Sets an array of indices that represent the order in which vertices will be drawn.
      The count of this array should be a multiple of 3 (for drawing triangles).
      OpenGL expects vertices to be in counter-clockwise orientation.
    */
    virtual void setIndexBuffer(const Shared<IndexBuffer>& indexBuffer) = 0;

    virtual const std::vector<Shared<VertexBuffer>>& getBuffers() const = 0;
    virtual const Shared<IndexBuffer>& getIndexBuffer() const = 0;


    static Shared<VertexArray> Create();
  };
}