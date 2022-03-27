#pragma once
#include "BufferLayout.h"

/*
  Abstract representation of a vertex array.
  Platform-specific implementation is determined by derived class.
*/
namespace Engine
{
  class VertexArray
  {
  public:
    virtual ~VertexArray() = default;

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual void setLayout(const BufferLayout& layout) = 0;

    /*
      \param data Buffer of vertex data
      \param size Size of buffer in bytes
    */
    virtual void setVertexData(const void* data, uintptr_t size) = 0;

    /*
      Sets an array of indices that represent the order in which vertices will be drawn.
      The count of this array should be a multiple of 3 (for drawing triangles).
      OpenGL expects vertices to be in counter-clockwise orientation.
    */
    virtual void setIndexBuffer(const uint32_t* indices, uint32_t count) = 0;

    virtual uint32_t getIndexCount() const = 0;


    static Unique<VertexArray> Create();
  };
}