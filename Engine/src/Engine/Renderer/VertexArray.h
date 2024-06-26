#pragma once
#include "Engine/Memory/BufferLayout.h"
#include "Engine/Memory/DynamicBuffer.h"

namespace eng
{
  /*
    Abstract representation of an index buffer.
    NOTE: Currently Engine only supports 32-bit index buffers
  */
  class IndexBuffer
  {
    std::shared_ptr<mem::DynamicBuffer> m_Buffer;

  public:
    IndexBuffer(const mem::IndexData& data);
    IndexBuffer(const std::shared_ptr<mem::DynamicBuffer>& indexBuffer);

    void bind() const;
    void unbind() const;

    uSize count() const;
  };



  /*
    Abstract representation of a vertex array.
    Platform-specific implementation is determined by derived class.
  */
  class VertexArray
  {
  public:
    virtual ~VertexArray();

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void setLayout(const mem::BufferLayout& layout) = 0;

    /*
      \param data Buffer of vertex data
      \param size Size of buffer in bytes
    */
    virtual void setVertexBuffer(const mem::RenderData& data) = 0;
    virtual void setVertexBuffer(const std::shared_ptr<mem::DynamicBuffer>& vertexBuffer) = 0;
    virtual void modifyVertexBuffer(u32 offset, const mem::RenderData& data) = 0;
    virtual void resizeVertexBuffer(uSize newSize) = 0;

    /*
      Sets an array of indices that represent the order in which vertices will be drawn.
      The count of this array should be a multiple of 3 (for drawing triangles).
      OpenGL expects vertices to be in counter-clockwise orientation.
    */
    virtual void setIndexBuffer(const IndexBuffer& indexBuffer) = 0;
    virtual void setIndexBuffer(const std::shared_ptr<mem::DynamicBuffer>& indexBuffer) = 0;

    virtual const mem::BufferLayout& layout() const = 0;
    virtual const std::optional<IndexBuffer>& indexBuffer() const = 0;

    static std::unique_ptr<VertexArray> Create();
  };
}