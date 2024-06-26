#pragma once
#include "Data.h"
#include "BufferLayout.h"

namespace eng::mem
{
  class DynamicBuffer
  {
  public:
    enum class Type
    {
      Vertex,
      Index
    };

    virtual ~DynamicBuffer();

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual Type type() const = 0;
    virtual uSize size() const = 0;

    virtual void set(const mem::RenderData& data) = 0;
    virtual void modify(u32 offset, const mem::RenderData& data) = 0;
    virtual void resize(uSize newSize) = 0;

    static std::unique_ptr<DynamicBuffer> Create(Type type);
  };
}