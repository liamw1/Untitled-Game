#pragma once
#include "Engine/Memory/BufferLayout.h"
#include "Engine/Memory/StorageBuffer.h"

namespace eng
{
  /*
    Abstract representation of an index buffer.
    NOTE: Currently Engine only supports 32-bit index buffers
  */
  class IndexBuffer
  {
    std::shared_ptr<StorageBuffer> m_Buffer;

  public:
    IndexBuffer(const mem::IndexData& data);
    IndexBuffer(const std::shared_ptr<StorageBuffer>& indexBufferStorage);

    void bind() const;
    void unBind() const;

    u32 count() const;
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
    virtual void unBind() const = 0;

    virtual void setLayout(const BufferLayout& layout) = 0;

    /*
      \param data Buffer of vertex data
      \param size Size of buffer in bytes
    */
    virtual void setVertexBuffer(const mem::Data& data) = 0;
    virtual void setVertexBuffer(const std::shared_ptr<StorageBuffer>& vertexBuffer) = 0;
    virtual void updateVertexBuffer(u32 offset, const mem::Data& data) const = 0;
    virtual void resizeVertexBuffer(u32 newSize) = 0;

    /*
      Sets an array of indices that represent the order in which vertices will be drawn.
      The count of this array should be a multiple of 3 (for drawing triangles).
      OpenGL expects vertices to be in counter-clockwise orientation.
    */
    virtual void setIndexBuffer(const IndexBuffer& indexBuffer) = 0;
    virtual void setIndexBuffer(const std::shared_ptr<StorageBuffer>& indexBufferStorage) = 0;

    virtual const BufferLayout& getLayout() const = 0;
    virtual const std::optional<IndexBuffer>& getIndexBuffer() const = 0;

    static std::unique_ptr<VertexArray> Create();
  };
}