#pragma once
#include "Buffer.h"

namespace Engine
{
  class VertexArray
  {
  public:
    virtual ~VertexArray() {}

    virtual void bind() const = 0;
    virtual void unBind() const = 0;

    virtual void addVertexBuffer(const Shared<VertexBuffer>& vertexBuffer) = 0;
    virtual void setIndexBuffer(const Shared<IndexBuffer>& indexBuffer) = 0;

    virtual const std::vector<Shared<VertexBuffer>>& getBuffers() const = 0;
    virtual const Shared<IndexBuffer>& getIndexBuffer() const = 0;


    static Shared<VertexArray> Create();
  };
}