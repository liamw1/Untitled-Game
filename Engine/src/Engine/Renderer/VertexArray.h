#pragma once
#include "BufferLayout.h"

namespace Engine
{
  /*
    Abstract representation of an index buffer.
    Platform-specific implementation is determined by derived class.
    NOTE: Currently Engine only supports 32-bit index buffers
  */
  class IndexBuffer
  {
  public:
    virtual ~IndexBuffer() = default;

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual uint32_t getCount() const = 0;

    /*
      \param indices Buffer of vertex indices
      \param count   Number of indices
    */
    static std::unique_ptr<IndexBuffer> Create(const uint32_t* indices, uint32_t count);
    static std::unique_ptr<IndexBuffer> Create(const std::vector<uint32_t>& indices);
  };



  /*
    Abstract representation of a vertex array.
    Platform-specific implementation is determined by derived class.
  */
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
    virtual void setVertexBuffer(const void* data, uintptr_t size) const = 0;

    /*
      Sets an array of indices that represent the order in which vertices will be drawn.
      The count of this array should be a multiple of 3 (for drawing triangles).
      OpenGL expects vertices to be in counter-clockwise orientation.
    */
    virtual void setIndexBuffer(const std::shared_ptr<const IndexBuffer>& indexBuffer) = 0;

    virtual const std::shared_ptr<const IndexBuffer>& getIndexBuffer() const = 0;

    static std::unique_ptr<VertexArray> Create();
  };
}